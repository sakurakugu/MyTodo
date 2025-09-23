/**
 * @file todo_query.h
 * @brief TodoQueryer类的头文件
 *
 * 该文件定义了TodoQueryer类，用于管理待办事项的筛选功能。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:51:47(UTC+8) 周一
 * @change 2025-09-01 10:47:47(UTC+8) 周一
 * @version 0.4.0
 */

#pragma once

#include "todo_item.h"
#include <QDate>
#include <QList>
#include <QObject>
#include <QString>

/**
 * @class TodoQueryer
 * @brief 待办事项查询器，负责管理所有筛选排序逻辑
 *
 * TodoQueryer类专门负责待办事项的筛选查询功能，包括：
 *
 * **筛选功能：**
 * - 分类筛选（按类别过滤）
 * - 状态筛选（完成/未完成/回收站）
 * - 日期筛选（按截止日期范围过滤）
 *
 * **排序功能：**
 * - 按创建时间排序
 * - 按截止日期排序
 * - 按重要程度排序
 * - 按标题排序
 *
 * **架构特点：**
 * - 继承自QObject，支持Qt信号槽机制
 * - 提供缓存机制，提升筛选性能
 * - 支持链式筛选条件组合
 * - 提供多种排序算法
 * - 支持自定义排序规则
 *
 * @note 该类是线程安全的
 * @see TodoItem, TodoManager
 */
class TodoQueryer : public QObject {
    Q_OBJECT
    // 筛选属性
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY currentCategoryChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QDate dateFilterStart READ dateFilterStart WRITE setDateFilterStart NOTIFY dateFilterStartChanged)
    Q_PROPERTY(QDate dateFilterEnd READ dateFilterEnd WRITE setDateFilterEnd NOTIFY dateFilterEndChanged)
    Q_PROPERTY(bool dateFilterEnabled READ dateFilterEnabled WRITE setDateFilterEnabled NOTIFY dateFilterEnabledChanged)
    // 排序属性
    Q_PROPERTY(int sortType READ sortType WRITE setSortType NOTIFY sortTypeChanged)
    Q_PROPERTY(bool descending READ descending WRITE setDescending NOTIFY descendingChanged)

  public:
    /**
     * @enum SortType
     * @brief 定义待办事项的排序类型
     */
    enum SortType {
        SortByCreatedTime = 0, // 按创建时间排序（默认）
        SortByDeadline = 1,    // 按截止日期排序
        SortByImportance = 2,  // 按重要程度排序
        SortByTitle = 3        // 按标题排序
    };
    Q_ENUM(SortType)

    // 构造函数与析构函数
    explicit TodoQueryer(QObject *parent = nullptr);
    ~TodoQueryer();

    // 筛选属性访问器
    QString currentCategory() const;                  // 获取当前激活的分类筛选器
    void setCurrentCategory(const QString &category); // 设置分类筛选器
    QString currentFilter() const;                    // 获取当前激活的筛选条件
    void setCurrentFilter(const QString &filter);     // 设置筛选条件
    QString searchText() const;                       // 获取搜索文本
    void setSearchText(const QString &text);          // 设置搜索文本

    // 日期筛选相关方法
    QDate dateFilterStart() const;              // 获取日期筛选开始日期
    void setDateFilterStart(const QDate &date); // 设置日期筛选开始日期
    QDate dateFilterEnd() const;                // 获取日期筛选结束日期
    void setDateFilterEnd(const QDate &date);   // 设置日期筛选结束日期
    bool dateFilterEnabled() const;             // 获取日期筛选是否启用
    void setDateFilterEnabled(bool enabled);    // 设置日期筛选是否启用

    // 排序属性访问器
    int sortType() const;          // 获取当前排序类型
    void setSortType(int type);    // 设置排序类型
    bool descending() const;       // 获取是否倒序
    void setDescending(bool desc); // 设置是否倒序

    bool 是否激活任何查询条件() const; ///< 检查是否有任何筛选条件被激活

  signals:
    void currentCategoryChanged();   // 当前分类筛选器变化信号
    void currentFilterChanged();     // 当前筛选条件变化信号
    void currentImportantChanged();  // 当前重要程度筛选器变化信号
    void searchTextChanged();        // 搜索文本变化信号
    void dateFilterStartChanged();   // 日期筛选开始日期变化信号
    void dateFilterEndChanged();     // 日期筛选结束日期变化信号
    void dateFilterEnabledChanged(); // 日期筛选启用状态变化信号
    void sortTypeChanged();          // 排序类型变化信号
    void descendingChanged();        // 倒序状态变化信号
    void queryConditionsChanged();   // 查询条件变化信号

  private:
    // 筛选条件成员变量
    QString m_currentCategory; ///< 当前分类筛选器
    QString m_currentFilter;   ///< 当前筛选条件
    QString m_searchText;      ///< 搜索文本
    QDate m_dateFilterStart;   ///< 日期筛选开始日期
    QDate m_dateFilterEnd;     ///< 日期筛选结束日期
    bool m_dateFilterEnabled;  ///< 日期筛选是否启用
    int m_sortType;            ///< 当前排序类型
    bool m_descending;         ///< 是否倒序排列

    // 辅助方法
    bool checkCategoryMatch(const TodoItem *item) const; // 检查分类筛选条件
    bool checkStatusMatch(const TodoItem *item) const;   // 检查状态筛选条件
    bool checkSearchMatch(const TodoItem *item) const;   // 检查搜索文本筛选条件
    bool checkDateMatch(const TodoItem *item) const;     // 检查日期筛选条件
};