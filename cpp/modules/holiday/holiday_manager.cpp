/**
 * @file holiday_manager.cpp
 * @brief HolidayManager类的实现
 *
 * 该文件实现了HolidayManager类的所有方法。
 *
 * @author Sakurakugu
 * @date 2025-10-07 23:33:00(UTC+8) 周二
 */

#include "holiday_manager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

// 常量定义
const QString HolidayManager::HOLIDAY_API_BASE_URL = "https://api.jiejiariapi.com/v1/holidays";
const int HolidayManager::UPDATE_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000; // 24小时
const QString HolidayManager::HOLIDAY_DATA_DIR = "data/holiday";

HolidayManager::HolidayManager(QObject *parent)
    : QObject(parent),                                  //
      m_updateTimer(new QTimer(this)),                  //
      m_networkRequest(&NetworkRequest::GetInstance()), //
      m_initialized(false) {

    m_holidayDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + HOLIDAY_DATA_DIR;

    // 设置定时器
    设置定时器();
}

/**
 * @brief 初始化管理器
 * @return 是否成功
 */
bool HolidayManager::初始化() {
    if (m_initialized)
        return true;

    qDebug() << "初始化节假日管理器...";

    // 创建数据目录
    QDir dir;
    if (!dir.exists(m_holidayDataDir)) {
        if (!dir.mkpath(m_holidayDataDir)) {
            qWarning() << "无法创建节假日数据目录:" << m_holidayDataDir;
            return false;
        }
    }

    加载本地节假日数据();

    // 检查是否需要更新数据
    std::vector<int> requiredYears = 获取需要管理的年份列表();
    for (const int year : requiredYears) {
        if (是否需要更新节假日数据(year)) {
            qDebug() << "需要更新" << year << "年的节假日数据";
            从网络获取节假日数据(year);
        }
    }

    m_initialized = true;
    emit holidayDataLoaded();

    qDebug() << "节假日管理器初始化完成";
    return true;
}

/**
 * @brief 获取指定日期的类型
 * @param date 日期
 * @return 日期类型
 */
HolidayManager::DateType HolidayManager::获取日期类型(const QDate &date) const {
    if (!date.isValid()) {
        return DateType::WorkDay;
    }

    int year = date.year();
    if (!m_holidayData.contains(year)) {
        // 如果没有该年份的数据，根据星期判断
        int dayOfWeek = date.dayOfWeek();
        return (dayOfWeek == Qt::Saturday || dayOfWeek == Qt::Sunday) ? Weekend : WorkDay;
    }

    const std::vector<HolidayItem> &holidays = m_holidayData[year];
    for (const HolidayItem &holiday : holidays) {
        if (holiday.date() == date) {
            return holiday.isOffDay() ? Holiday : HolidayWork;
        }
    }

    // 不是节假日，检查是否为周末
    int dayOfWeek = date.dayOfWeek();
    return (dayOfWeek == Qt::Saturday || dayOfWeek == Qt::Sunday) ? Weekend : WorkDay;
}

/**
 * @brief 检查是否为节假日（放假日）
 * @param date 日期
 * @return 是否为节假日
 */
bool HolidayManager::是否为节假日(const QDate &date) const {
    return 获取日期类型(date) == Holiday;
}

/**
 * @brief 检查是否为工作日
 * @param date 日期
 * @return 是否为工作日
 */
bool HolidayManager::是否为工作日(const QDate &date) const {
    DateType type = 获取日期类型(date);
    return type == WorkDay || type == HolidayWork;
}

/**
 * @brief 检查是否为周末
 * @param date 日期
 * @return 是否为周末
 */
bool HolidayManager::是否为周末(const QDate &date) const {
    return 获取日期类型(date) == Weekend;
}

/**
 * @brief 获取指定日期的节假日名称
 * @param date 日期
 * @return 节假日名称，如果不是节假日返回空字符串
 */
QString HolidayManager::获取节假日名称(const QDate &date) const {
    if (!date.isValid()) {
        return QString();
    }

    int year = date.year();
    if (!m_holidayData.contains(year)) {
        return QString();
    }

    const std::vector<HolidayItem> &holidays = m_holidayData[year];
    for (const HolidayItem &holiday : holidays) {
        if (holiday.date() == date) {
            return holiday.name();
        }
    }

    return QString();
}

/**
 * @brief 获取指定年份的所有节假日
 * @param year 年份
 * @return 节假日列表
 */
std::vector<HolidayItem> HolidayManager::获取指定年份的所有节假日(int year) const {
    return m_holidayData.value(year, std::vector<HolidayItem>());
}

/**
 * @brief 手动刷新指定年份的节假日数据
 * @param year 年份
 */
void HolidayManager::手动刷新指定年份的节假日数据(int year) {
    qDebug() << "手动刷新" << year << "年的节假日数据";
    从网络获取节假日数据(year);
}

/**
 * @brief 获取下一个工作日
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的工作日数量
 * @return 下一个工作日
 */
QDate HolidayManager::获取下一个工作日(const QDate &fromDate, int daysToAdd) const {
    QDate currentDate = fromDate;
    int addedDays = 0;

    while (addedDays < daysToAdd) {
        currentDate = currentDate.addDays(1);
        if (是否为工作日(currentDate)) {
            addedDays++;
        }
    }

    return currentDate;
}

/**
 * @brief 获取下一个节假日
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的节假日数量
 * @return 下一个节假日
 */
QDate HolidayManager::获取下一个节假日(const QDate &fromDate, int daysToAdd) const {
    QDate currentDate = fromDate;
    int addedDays = 0;

    while (addedDays < daysToAdd) {
        currentDate = currentDate.addDays(1);
        DateType type = 获取日期类型(currentDate);
        if (type == Holiday || type == Weekend) {
            addedDays++;
        }
    }

    return currentDate;
}

/**
 * @brief 获取下一个周末
 * @param fromDate 起始日期
 * @param daysToAdd 要添加的周末数量
 * @return 下一个周末
 */
QDate HolidayManager::获取下一个周末(const QDate &fromDate, int daysToAdd) const {
    QDate currentDate = fromDate;
    int addedDays = 0;

    while (addedDays < daysToAdd) {
        currentDate = currentDate.addDays(1);
        if (是否为周末(currentDate)) {
            addedDays++;
        }
    }

    return currentDate;
}

/**
 * @brief 定时检查节假日数据
 */
void HolidayManager::checkHolidayDataPeriodically() {
    // 检查当前需要的所有年份
    std::vector<int> requiredYears = 获取需要管理的年份列表();
    for (const int year : requiredYears) {
        if (是否需要更新节假日数据(year)) {
            从网络获取节假日数据(year);
        }
    }
}

/**
 * @brief 加载本地节假日数据
 * @return 是否成功
 */
bool HolidayManager::加载本地节假日数据() {
    std::vector<int> requiredYears = 获取需要管理的年份列表();
    bool success = true;

    for (const int year : requiredYears) {
        std::vector<HolidayItem> holidays = 从本地文件加载节假日数据(year);
        if (!holidays.empty()) {
            m_holidayData[year] = holidays;
            qDebug() << "加载" << year << "年节假日数据成功，共" << holidays.size() << "条";
        } else {
            qDebug() << "未找到" << year << "年的本地节假日数据";
            success = false;
        }
    }

    return success;
}

/**
 * @brief 保存节假日数据到本地文件
 * @param year 年份
 * @param holidays 节假日列表
 * @return 是否成功
 */
bool HolidayManager::保存节假日数据到本地文件(int year, const std::vector<HolidayItem> &holidays) {
    QString filePath = 获取节假日数据文件路径(year);

    QJsonObject jsonObject;
    QJsonArray holidayArray;

    for (const HolidayItem &holiday : holidays) {
        holidayArray.append(holiday.toJson());
    }

    jsonObject["year"] = year;
    jsonObject["holidays"] = holidayArray;
    jsonObject["updateTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(jsonObject);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();

    return true;
}

/**
 * @brief 从本地文件加载节假日数据
 * @param year 年份
 * @return 节假日列表
 */
std::vector<HolidayItem> HolidayManager::从本地文件加载节假日数据(int year) {
    QString filePath = 获取节假日数据文件路径(year);
    std::vector<HolidayItem> holidays;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return holidays; // 文件不存在或无法打开
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "解析节假日数据文件失败:" << error.errorString();
        return holidays;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("holidays") || !jsonObject["holidays"].isArray()) {
        qWarning() << "节假日数据文件格式错误";
        return holidays;
    }

    QJsonArray holidayArray = jsonObject["holidays"].toArray();
    for (const QJsonValue &value : holidayArray) {
        if (value.isObject()) {
            HolidayItem holiday(value.toObject());
            if (holiday.isValid()) {
                holidays.push_back(holiday);
            }
        }
    }

    return holidays;
}

/**
 * @brief 获取节假日数据文件路径
 * @param year 年份
 * @return 文件路径
 */
QString HolidayManager::获取节假日数据文件路径(int year) const {
    return m_holidayDataDir + QString("/%1_holidays.json").arg(year);
}

/**
 * @brief 从网络获取节假日数据
 * @param year 年份
 */
void HolidayManager::从网络获取节假日数据(int year) {
    QString url = QString("%1/%2").arg(HOLIDAY_API_BASE_URL).arg(year);

    NetworkRequest::RequestConfig config;
    config.url = url;
    config.method = "GET";
    config.requiresAuth = false; // 节假日API不需要认证
    config.timeout = 15000;      // 15秒超时

    // 创建自定义响应处理器，用于传递年份信息
    auto customHandler = [this, year](const QByteArray &rawResponse, int httpStatusCode) -> QJsonObject {
        QJsonObject result;

        if (httpStatusCode == 200) {
            // 首先检查原始响应是否是纯字符串（如"Year not found"）
            QString responseText = QString::fromUtf8(rawResponse).trimmed();
            
            // 如果响应不以 '{' 开头，很可能是错误字符串而不是JSON
            if (!responseText.startsWith('{')) {
                qWarning() << "API返回错误信息:" << responseText << "（年份:" << year << "）";
                
                // 记录最后检查时间，避免重复请求
                m_lastUpdateCheck[year] = QDate::currentDate();
                emit holidayDataUpdated(year, false);
                
                result["success"] = false;
                result["error"] = QString("API错误: %1").arg(responseText);
            } else {
                // 尝试解析JSON
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(rawResponse, &parseError);

                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    // 正常的JSON对象响应
                    QJsonObject apiResponse = doc.object();

                    // 解析节假日数据
                    std::vector<HolidayItem> holidays = 解析API响应数据(year, apiResponse);

                    if (!holidays.empty()) {
                        // 保存到内存和文件
                        m_holidayData[year] = holidays;
                        if (保存节假日数据到本地文件(year, holidays)) {
                            qDebug() << "成功保存" << year << "年节假日数据，共" << holidays.size() << "条";
                            m_lastUpdateCheck[year] = QDate::currentDate();
                            emit holidayDataUpdated(year, true);
                        } else {
                            qWarning() << "保存" << year << "年节假日数据失败";
                            emit holidayDataUpdated(year, false);
                        }

                        result["success"] = true;
                        result["year"] = year;
                        result["count"] = static_cast<int>(holidays.size());
                    } else {
                        qWarning() << "解析" << year << "年节假日数据失败";
                        emit holidayDataUpdated(year, false);
                        result["success"] = false;
                        result["error"] = "解析节假日数据失败";
                    }
                } else {
                    qWarning() << "解析" << year << "年节假日响应JSON失败:" << parseError.errorString();
                    qWarning() << "原始响应内容:" << rawResponse;
                    emit holidayDataUpdated(year, false);
                    result["success"] = false;
                    result["error"] = QString("JSON解析失败: %1").arg(parseError.errorString());
                }
            }
        } else {
            qWarning() << "获取" << year << "年节假日数据失败，HTTP状态码:" << httpStatusCode;
            emit holidayDataUpdated(year, false);
            result["success"] = false;
            result["error"] = QString("HTTP错误，状态码: %1").arg(httpStatusCode);
        }

        return result;
    };

    qDebug() << "请求" << year << "年节假日数据:" << url;
    m_networkRequest->sendRequest(Network::RequestType::CheckHoliday, config, customHandler);
}

/**
 * @brief 解析API响应数据
 * @param year 年份
 * @param response API响应
 * @return 节假日列表
 */
std::vector<HolidayItem> HolidayManager::解析API响应数据(int year, const QJsonObject &response) {
    std::vector<HolidayItem> holidays;

    // API返回的是一个对象，每个键是日期，值是节假日信息
    for (auto it = response.begin(); it != response.end(); ++it) {
        QString dateStr = it.key();
        QJsonValue value = it.value();

        if (!value.isObject()) {
            continue;
        }

        QJsonObject holidayObj = value.toObject();
        HolidayItem holiday(holidayObj);

        if (holiday.isValid() && holiday.date().year() == year) {
            holidays.push_back(holiday);
        }
    }

    // 按日期排序
    std::sort(holidays.begin(), holidays.end());

    return holidays;
}

/**
 * @brief 检查是否需要更新节假日数据
 * @param year 年份
 * @return 是否需要更新
 */
bool HolidayManager::是否需要更新节假日数据(int year) const {
    // 检查是否有本地数据，如果没有则需要从网络获取
    if (!m_holidayData.contains(year) || m_holidayData[year].empty())
        return true;

    // 检查文件是否存在，如果不存在则需要从网络获取
    QString filePath = 获取节假日数据文件路径(year);
    if (!QFile::exists(filePath))
        return true;

    // 检查今天是否已经检查过这个year的数据（避免重复请求）
    QDate currentDate = QDate::currentDate();
    if (m_lastUpdateCheck.contains(year) && m_lastUpdateCheck[year] == currentDate)
        return false;

    // 如果是下一年，且现在是12月，要检查下一年的数据
    if (year == currentDate.year() + 1 && currentDate.month() == 12)
        return true;

    return false;
}

/**
 * @brief 设置定时器
 */
void HolidayManager::设置定时器() {
    // 设置定时器，每24小时检查一次
    m_updateTimer->setInterval(UPDATE_CHECK_INTERVAL_MS);
    m_updateTimer->setSingleShot(false);

    connect(m_updateTimer, &QTimer::timeout, this, &HolidayManager::checkHolidayDataPeriodically);

    // 启动定时器
    m_updateTimer->start();

    // 不用立即执行一次检查，等定时器触发，第一次执行在初始化中已经执行了
}

/**
 * @brief 获取需要管理的年份列表
 * @return 年份列表
 */
std::vector<int> HolidayManager::获取需要管理的年份列表() const {
    QDate currentDate = QDate::currentDate();
    int currentYear = currentDate.year();

    std::vector<int> years;
    years.push_back(currentYear - 1); // 前年
    years.push_back(currentYear);     // 当年

    // 如果是12月，也包含明年（一般11月中旬发布第二年的节假日安排）
    if (currentDate.month() == 12) {
        years.push_back(currentYear + 1);
    }

    return years;
}