/**
 * @file holiday_manager.h
 * @brief HolidayManager类的头文件
 *
 * 该文件定义了HolidayManager类，用于管理节假日数据的获取、存储和查询。
 *
 * @author Sakurakugu
 * @date 2025-10-07 23:33:00(UTC+8) 周二
 */

#pragma once

#include "holiday_item.h"
#include "network_request.h"
#include <QObject>
#include <QTimer>
#include <unordered_map>
#include <nlohmann/json.hpp>

// 前向声明
class NetworkRequest;

/**
 * @class HolidayManager
 * @brief 节假日管理器类
 *
 * HolidayManager类负责管理节假日数据：
 *
 * **核心功能：**
 * - 从网络API获取节假日数据
 * - 本地文件存储和读取
 * - 自动更新机制（12月检查下一年数据）
 * - 节假日查询和判断
 *
 * **数据管理：**
 * - 保存当年、前年、明年的节假日数据
 * - 支持工作日、节假日、周末的判断
 * - 提供日期类型查询接口
 *
 * **自动更新：**
 * - 12月份每天检查一次下一年数据
 * - 避免重复请求（每天只请求一次）
 * - 网络请求失败时的重试机制
 *
 * **使用场景：**
 * - QML DateTimePicker显示节假日信息
 * - RecurrenceInterval计算工作日重复
 * - 应用程序节假日相关功能
 */
class HolidayManager : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief 日期类型枚举
     */
    enum DateType {
        WorkDay = 0,    ///< 工作日
        Holiday = 1,    ///< 节假日（放假）
        Weekend = 2,    ///< 周末
        HolidayWork = 3 ///< 节假日调休（上班）
    };

    // 单例模式
    static HolidayManager &GetInstance() {
        static HolidayManager instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    HolidayManager(const HolidayManager &) = delete;
    HolidayManager &operator=(const HolidayManager &) = delete;
    HolidayManager(HolidayManager &&) = delete;
    HolidayManager &operator=(HolidayManager &&) = delete;

    bool 初始化();
    DateType 获取日期类型(const my::Date &date) const;
    DateType getDateType(const my::Date &date) const { return 获取日期类型(date); }
    bool 是否为节假日(const my::Date &date) const;
    bool 是否为工作日(const my::Date &date) const;
    bool 是否为周末(const my::Date &date) const;
    std::string 获取节假日名称(const my::Date &date) const;
    std::vector<HolidayItem> 获取指定年份的所有节假日(int year) const;
    void 手动刷新指定年份的节假日数据(int year);
    my::Date 获取下一个工作日(const my::Date &fromDate, int daysToAdd = 1) const;
    my::Date 获取下一个节假日(const my::Date &fromDate, int daysToAdd = 1) const;
    my::Date 获取下一个周末(const my::Date &fromDate, int daysToAdd = 1) const;

  signals:
    void holidayDataUpdated(int year, bool success); // 节假日数据更新完成信号
    void holidayDataLoaded();                        // 节假日数据加载完成信号

  private slots:
    void checkHolidayDataPeriodically(); // 定时检查节假日数据

  private:
    explicit HolidayManager(QObject *parent = nullptr);
    ~HolidayManager() = default;

    bool 加载本地节假日数据();
    bool 保存节假日数据到本地文件(int year, const std::vector<HolidayItem> &holidays);
    std::vector<HolidayItem> 从本地文件加载节假日数据(int year);
    std::string 获取节假日数据文件路径(int year) const;
    void 从网络获取节假日数据(int year);
    std::vector<HolidayItem> 解析API响应数据(int year, const nlohmann::json &response);
    bool 是否需要更新节假日数据(int year) const;
    void 设置定时器();

    std::vector<int> 获取需要管理的年份列表() const;

    // 成员变量
    std::unordered_map<int, std::vector<HolidayItem>> m_holidayData; ///< 节假日数据缓存 (年份 -> 节假日列表)
    QTimer *m_updateTimer;                                           ///< 定时更新定时器
    std::unordered_map<int, my::Date> m_lastUpdateCheck;                       ///< 最后检查更新的日期 (年份 -> 日期)
    NetworkRequest *m_networkRequest;                                ///< 网络请求管理器
    bool m_initialized;                                              ///< 是否已初始化
    std::string m_holidayDataDir;                                    ///< 节假日数据目录

    // 常量
    static const std::string HOLIDAY_API_BASE_URL; ///< 节假日API基础URL
    static const int UPDATE_CHECK_INTERVAL_MS;     ///< 更新检查间隔（毫秒）
    static const std::string HOLIDAY_DATA_DIR;     ///< 节假日数据目录
};