/**
 * @file todo_filter.h
 * @brief TodoFilter类的头文件
 *
 * 该文件定义了TodoFilter类，用于管理待办事项的筛选功能。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#pragma once

#include "items/todo_item.h"
#include <QDate>
#include <QList>
#include <QObject>
#include <QString>

/**
 * @class TodoFilter
 * @brief 待办事项筛选器，负责管理所有筛选逻辑
 *
 * TodoFilter类专门负责待办事项的筛选功能，包括：
 *
 * **筛选功能：**
 * - 分类筛选（按类别过滤）
 * - 状态筛选（完成/未完成/回收站）
 * - 日期筛选（按截止日期范围过滤）
 *
 * **架构特点：**
 * - 继承自QObject，支持Qt信号槽机制
 * - 提供缓存机制，提升筛选性能
 * - 支持链式筛选条件组合
 *
 * @note 该类是线程安全的
 * @see TodoItem, TodoManager
 */
class TodoFilter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY currentCategoryChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)
    Q_PROPERTY(QDate dateFilterStart READ dateFilterStart WRITE setDateFilterStart NOTIFY dateFilterStartChanged)
    Q_PROPERTY(QDate dateFilterEnd READ dateFilterEnd WRITE setDateFilterEnd NOTIFY dateFilterEndChanged)
    Q_PROPERTY(bool dateFilterEnabled READ dateFilterEnabled WRITE setDateFilterEnabled NOTIFY dateFilterEnabledChanged)

  public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TodoFilter(QObject *parent = nullptr);
    ~TodoFilter();

    // 筛选属性访问器
    QString currentCategory() const;                  // 获取当前激活的分类筛选器
    void setCurrentCategory(const QString &category); // 设置分类筛选器
    QString currentFilter() const;                    // 获取当前激活的筛选条件
    void setCurrentFilter(const QString &filter);     // 设置筛选条件
    void setCurrentImportant(bool important);         // 设置重要程度筛选器

    // 日期筛选相关方法
    QDate dateFilterStart() const;              // 获取日期筛选开始日期
    void setDateFilterStart(const QDate &date); // 设置日期筛选开始日期
    QDate dateFilterEnd() const;                // 获取日期筛选结束日期
    void setDateFilterEnd(const QDate &date);   // 设置日期筛选结束日期
    bool dateFilterEnabled() const;             // 获取日期筛选是否启用
    void setDateFilterEnabled(bool enabled);    // 设置日期筛选是否启用

    // 筛选核心方法
    bool itemMatchesFilter(const TodoItem *item) const; ///< 检查项目是否匹配当前筛选条件
    QList<TodoItem *> filterTodos(
        const std::vector<std::unique_ptr<TodoItem>> &todos) const; ///< 从待办事项列表中筛选出匹配的项目
    void resetFilters();                                            ///< 重置所有筛选条件
    bool hasActiveFilters() const;                                  ///< 检查是否有任何筛选条件被激活

  signals:
    void currentCategoryChanged();   // 当前分类筛选器变化信号
    void currentFilterChanged();     // 当前筛选条件变化信号
    void currentImportantChanged();  // 当前重要程度筛选器变化信号
    void dateFilterStartChanged();   // 日期筛选开始日期变化信号
    void dateFilterEndChanged();     // 日期筛选结束日期变化信号
    void dateFilterEnabledChanged(); // 日期筛选启用状态变化信号
    void filtersChanged();           // 任何筛选条件变化的通用信号

  private:
    // 筛选条件成员变量
    QString m_currentCategory; ///< 当前分类筛选器
    QString m_currentFilter;   ///< 当前筛选条件
    QDate m_dateFilterStart;   ///< 日期筛选开始日期
    QDate m_dateFilterEnd;     ///< 日期筛选结束日期
    bool m_dateFilterEnabled;  ///< 日期筛选是否启用

    // 辅助方法
    bool checkCategoryMatch(const TodoItem *item) const; // 检查分类筛选条件
    bool checkStatusMatch(const TodoItem *item) const;   // 检查状态筛选条件
    bool checkDateMatch(const TodoItem *item) const;     // 检查日期筛选条件

    /**
     * @brief 发射筛选条件变化信号
     */
    void emitFiltersChanged();
};