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
#include "datetime.h"
#include <QStandardPaths>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// 常量定义
const std::string HolidayManager::HOLIDAY_API_BASE_URL = "https://api.jiejiariapi.com/v1/holidays";
const int HolidayManager::UPDATE_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000; // 24小时
const std::string HolidayManager::HOLIDAY_DATA_DIR = "data/holiday";

HolidayManager::HolidayManager(QObject *parent)
    : QObject(parent),                                  //
      m_updateTimer(new QTimer(this)),                  //
      m_networkRequest(&NetworkRequest::GetInstance()), //
      m_initialized(false) {

    m_holidayDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toStdString() + "/" + HOLIDAY_DATA_DIR;

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
    std::filesystem::path dirPath(m_holidayDataDir);
    if (!std::filesystem::exists(dirPath)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(dirPath, ec)) {
            qWarning() << "无法创建节假日数据目录:" << m_holidayDataDir.c_str() << "错误:" << ec.message().c_str();
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
HolidayManager::DateType HolidayManager::获取日期类型(const my::Date &date) const {
    if (!date.isValid()) {
        return DateType::WorkDay;
    }

    int year = date.year();
    if (!m_holidayData.contains(year)) {
        // 如果没有该年份的数据，根据星期判断
        uint8_t dayOfWeek = date.dayOfWeek();
        return (dayOfWeek == my::DayOfWeek::Saturday || dayOfWeek == my::DayOfWeek::Sunday) ? Weekend : WorkDay;
    }

    // 检查是否为节假日
    const std::vector<HolidayItem> &holidays = m_holidayData.find(year)->second;
    for (const HolidayItem &holiday : holidays) {
        if (holiday.date() == date) {
            return holiday.isOffDay() ? DateType::Holiday : DateType::HolidayWork;
        }
    }

    // 不是节假日，检查是否为周末
    uint8_t dayOfWeek = date.dayOfWeek();
    return (dayOfWeek == my::DayOfWeek::Saturday || dayOfWeek == my::DayOfWeek::Sunday) ? Weekend : WorkDay;
}

/**
 * @brief 检查是否为节假日（放假日）
 * @param date 日期
 * @return 是否为节假日
 */
bool HolidayManager::是否为节假日(const my::Date &date) const {
    return 获取日期类型(date) == DateType::Holiday;
}

/**
 * @brief 检查是否为工作日
 * @param date 日期
 * @return 是否为工作日
 */
bool HolidayManager::是否为工作日(const my::Date &date) const {
    DateType type = 获取日期类型(date);
    return type == DateType::WorkDay || type == DateType::HolidayWork;
}

/**
 * @brief 检查是否为周末
 * @param date 日期
 * @return 是否为周末
 */
bool HolidayManager::是否为周末(const my::Date &date) const {
    return 获取日期类型(date) == DateType::Weekend;
}

/**
 * @brief 获取指定日期的节假日名称
 * @param date 日期
 * @return 节假日名称，如果不是节假日返回空字符串
 */
std::string HolidayManager::获取节假日名称(const my::Date &date) const {
    if (!date.isValid()) {
        return std::string();
    }

    int year = date.year();
    if (!m_holidayData.contains(year)) {
        return std::string();
    }

    const std::vector<HolidayItem> &holidays = m_holidayData.find(year)->second;
    for (const HolidayItem &holiday : holidays) {
        if (holiday.date() == date) {
            return holiday.name();
        }
    }

    return std::string();
}

/**
 * @brief 获取指定年份的所有节假日
 * @param year 年份
 * @return 节假日列表
 */
std::vector<HolidayItem> HolidayManager::获取指定年份的所有节假日(int year) const {
    return m_holidayData.find(year)->second;
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
my::Date HolidayManager::获取下一个工作日(const my::Date &fromDate, int daysToAdd) const {
    my::Date currentDate = fromDate;
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
my::Date HolidayManager::获取下一个节假日(const my::Date &fromDate, int daysToAdd) const {
    my::Date currentDate = fromDate;
    int addedDays = 0;

    while (addedDays < daysToAdd) {
        currentDate = currentDate.addDays(1);
        DateType type = 获取日期类型(currentDate);
        if (type == DateType::Holiday || type == DateType::Weekend) {
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
my::Date HolidayManager::获取下一个周末(const my::Date &fromDate, int daysToAdd) const {
    my::Date currentDate = fromDate;
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
    std::string filePath = 获取节假日数据文件路径(year);

    nlohmann::json jsonObject;
    nlohmann::json holidayArray = nlohmann::json::array();

    for (const HolidayItem &holiday : holidays) {
        holidayArray.push_back(holiday.toJson());
    }

    jsonObject["year"] = year;
    jsonObject["holidays"] = holidayArray;
    jsonObject["updateTime"] = my::DateTime::now().toISOString();

    std::ofstream file(filePath);
    if (!file.is_open()) {
        qWarning() << "无法打开文件进行写入:" << filePath.c_str();
        return false;
    }

    file << jsonObject.dump(4); // 格式化输出，缩进4个空格
    file.close();

    return true;
}

/**
 * @brief 从本地文件加载节假日数据
 * @param year 年份
 * @return 节假日列表
 */
std::vector<HolidayItem> HolidayManager::从本地文件加载节假日数据(int year) {
    std::string filePath = 获取节假日数据文件路径(year);
    std::vector<HolidayItem> holidays;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return holidays; // 文件不存在或无法打开
    }

    try {
        nlohmann::json jsonObject;
        file >> jsonObject;
        file.close();

        if (!jsonObject.contains("holidays") || !jsonObject["holidays"].is_array()) {
            qWarning() << "节假日数据文件格式错误";
            return holidays;
        }

        const auto& holidayArray = jsonObject["holidays"];
        for (const auto& value : holidayArray) {
            if (value.is_object()) {
                HolidayItem holiday(value);
                if (holiday.isValid()) {
                    holidays.push_back(holiday);
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        qWarning() << "解析节假日数据文件失败:" << e.what();
        return holidays;
    }

    return holidays;
}

/**
 * @brief 获取节假日数据文件路径
 * @param year 年份
 * @return 文件路径
 */
std::string HolidayManager::获取节假日数据文件路径(int year) const {
    return m_holidayDataDir + "/" + std::to_string(year) + "_holidays.json";
}

/**
 * @brief 从网络获取节假日数据
 * @param year 年份
 */
void HolidayManager::从网络获取节假日数据(int year) {
    std::string url = HOLIDAY_API_BASE_URL + "/" + std::to_string(year);

    NetworkRequest::RequestConfig config;
    config.url = url.c_str();
    config.method = "GET";
    config.requiresAuth = false; // 节假日API不需要认证
    config.timeout = 15000;      // 15秒超时

    // 创建自定义响应处理器，用于传递年份信息
    auto customHandler = [this, year](const QByteArray &rawResponse, int httpStatusCode) -> nlohmann::json {
        nlohmann::json result;

        if (httpStatusCode == 200) {
            // 首先检查原始响应是否是纯字符串（如"Year not found"）
            std::string responseText = rawResponse.toStdString();

            // 如果响应不以 '{' 开头，很可能是错误字符串而不是JSON
            if (!responseText.starts_with('{')) {
                qWarning() << "API返回错误信息:" << responseText << "（年份:" << year << "）";

                // 记录最后检查时间，避免重复请求
                m_lastUpdateCheck[year] = my::Date::today();
                emit holidayDataUpdated(year, false);

                result["success"] = false;
                result["error"] = std::format("API错误: {}", responseText);
            } else {
                // 尝试解析JSON
                try {
                    nlohmann::json apiResponse = nlohmann::json::parse(rawResponse.toStdString());

                    // 解析节假日数据
                    std::vector<HolidayItem> holidays = 解析API响应数据(year, apiResponse);

                    if (!holidays.empty()) {
                        // 保存到内存和文件
                        m_holidayData[year] = holidays;
                        if (保存节假日数据到本地文件(year, holidays)) {
                            qDebug() << "成功保存" << year << "年节假日数据，共" << holidays.size() << "条";
                            m_lastUpdateCheck[year] = my::Date::today();
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
                } catch (const nlohmann::json::exception& e) {
                    qWarning() << "解析" << year << "年节假日响应JSON失败:" << e.what();
                    qWarning() << "原始响应内容:" << rawResponse;
                    emit holidayDataUpdated(year, false);
                    result["success"] = false;
                    result["error"] = std::format("JSON解析失败: {}", e.what());
                }
            }
        } else {
            qWarning() << "获取" << year << "年节假日数据失败，HTTP状态码:" << httpStatusCode;
            emit holidayDataUpdated(year, false);
            result["success"] = false;
            result["error"] = std::format("HTTP错误，状态码: {}", httpStatusCode);
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
std::vector<HolidayItem> HolidayManager::解析API响应数据(int year, const nlohmann::json &response) {
    std::vector<HolidayItem> holidays;

    // API返回的是一个对象，每个键是日期，值是节假日信息
    for (auto it = response.begin(); it != response.end(); ++it) {
        std::string dateStr = it.key();
        const auto& value = it.value();

        if (!value.is_object()) {
            continue;
        }

        HolidayItem holiday(value);

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
    if (!m_holidayData.contains(year) || m_holidayData.find(year)->second.empty())
        return true;

    // 检查文件是否存在，如果不存在则需要从网络获取
    std::string filePath = 获取节假日数据文件路径(year);
    if (!std::filesystem::exists(filePath))
        return true;

    // 检查今天是否已经检查过这个year的数据（避免重复请求）
    my::Date currentDate = my::Date::today();
    if (m_lastUpdateCheck.contains(year) && m_lastUpdateCheck.find(year)->second == currentDate)
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
    my::Date currentDate = my::Date::today();
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