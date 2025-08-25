/**
 * @file todo_filter.cpp
 * @brief TodoFilter类的实现文件
 *
 * 该文件实现了TodoFilter类的所有筛选功能。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#include "todo_filter.h"
#include <QDebug>

TodoFilter::TodoFilter(QObject *parent)
    : QObject(parent),           //
      m_currentCategory(""),     //
      m_currentFilter(""),       //
      m_dateFilterEnabled(false) //
{
}

TodoFilter::~TodoFilter() {
}

/**
 * @brief 获取当前激活的分类筛选器
 * @return 当前分类名称
 */
QString TodoFilter::currentCategory() const {
    return m_currentCategory;
}

/**
 * @brief 设置分类筛选器
 * @param category 分类名称，空字符串表示显示所有分类
 */
void TodoFilter::setCurrentCategory(const QString &category) {
    if (m_currentCategory != category) {
        m_currentCategory = category;
        emit currentCategoryChanged();
        emitFiltersChanged();
    }
}

/**
 * @brief 获取当前激活的筛选条件
 * @return 当前筛选条件
 */
QString TodoFilter::currentFilter() const {
    return m_currentFilter;
}

/**
 * @brief 设置筛选条件
 * @param filter 筛选条件（如"done"、"todo"等）
 */
void TodoFilter::setCurrentFilter(const QString &filter) {
    if (m_currentFilter != filter) {
        m_currentFilter = filter;
        emit currentFilterChanged();
        emitFiltersChanged();
    }
}

/**
 * @brief 获取日期筛选开始日期
 * @return 日期筛选开始日期
 */
QDate TodoFilter::dateFilterStart() const {
    return m_dateFilterStart;
}

/**
 * @brief 设置日期筛选开始日期
 * @param date 开始日期
 */
void TodoFilter::setDateFilterStart(const QDate &date) {
    if (m_dateFilterStart != date) {
        m_dateFilterStart = date;
        emit dateFilterStartChanged();
        emitFiltersChanged();
    }
}

/**
 * @brief 获取日期筛选结束日期
 * @return 日期筛选结束日期
 */
QDate TodoFilter::dateFilterEnd() const {
    return m_dateFilterEnd;
}

/**
 * @brief 设置日期筛选结束日期
 * @param date 结束日期
 */
void TodoFilter::setDateFilterEnd(const QDate &date) {
    if (m_dateFilterEnd != date) {
        m_dateFilterEnd = date;
        emit dateFilterEndChanged();
        emitFiltersChanged();
    }
}

/**
 * @brief 获取日期筛选是否启用
 * @return 日期筛选是否启用
 */
bool TodoFilter::dateFilterEnabled() const {
    return m_dateFilterEnabled;
}

/**
 * @brief 设置日期筛选是否启用
 * @param enabled 是否启用日期筛选
 */
void TodoFilter::setDateFilterEnabled(bool enabled) {
    if (m_dateFilterEnabled != enabled) {
        m_dateFilterEnabled = enabled;
        emit dateFilterEnabledChanged();
        emitFiltersChanged();
    }
}

/**
 * @brief 检查项目是否匹配当前筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoFilter::itemMatchesFilter(const TodoItem *item) const {
    if (!item)
        return false;

    return checkCategoryMatch(item) && checkStatusMatch(item) && checkDateMatch(item);
}

/**
 * @brief 从待办事项列表中筛选出匹配的项目
 * @param todos 原始待办事项列表
 * @return 筛选后的待办事项列表
 */
QList<TodoItem *> TodoFilter::filterTodos(const std::vector<std::unique_ptr<TodoItem>> &todos) const {
    QList<TodoItem *> filteredTodos;

    for (const auto &todo : todos) {
        if (itemMatchesFilter(todo.get())) {
            filteredTodos.append(todo.get());
        }
    }

    return filteredTodos;
}

/**
 * @brief 重置所有筛选条件
 */
void TodoFilter::resetFilters() {
    bool changed = false;

    if (!m_currentCategory.isEmpty()) {
        m_currentCategory.clear();
        emit currentCategoryChanged();
        changed = true;
    }

    if (!m_currentFilter.isEmpty()) {
        m_currentFilter.clear();
        emit currentFilterChanged();
        changed = true;
    }

    if (m_dateFilterEnabled) {
        m_dateFilterEnabled = false;
        emit dateFilterEnabledChanged();
        changed = true;
    }

    if (m_dateFilterStart.isValid()) {
        m_dateFilterStart = QDate();
        emit dateFilterStartChanged();
        changed = true;
    }

    if (m_dateFilterEnd.isValid()) {
        m_dateFilterEnd = QDate();
        emit dateFilterEndChanged();
        changed = true;
    }

    if (changed) {
        emitFiltersChanged();
    }
}

/**
 * @brief 检查是否有任何筛选条件被激活
 * @return 如果有筛选条件被激活返回true，否则返回false
 */
bool TodoFilter::hasActiveFilters() const {
    return !m_currentCategory.isEmpty() || !m_currentFilter.isEmpty() || m_dateFilterEnabled;
}

/**
 * @brief 检查分类筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoFilter::checkCategoryMatch(const TodoItem *item) const {
    if (m_currentCategory.isEmpty()) {
        return true; // 没有分类筛选，显示全部
    }

    return item->category() == m_currentCategory;
}

/**
 * @brief 检查状态筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoFilter::checkStatusMatch(const TodoItem *item) const {
    // 状态筛选逻辑：
    // - 如果m_currentFilter为"recycle"，则只显示已删除的项目
    // - 否则只显示未删除的项目，并根据完成状态进一步筛选
    // - 如果m_currentFilter为空，则显示所有项目
    if (m_currentFilter.isEmpty()) {
        return true; // 没有状态筛选，显示所有
    } else if (m_currentFilter == "recycle") {
        // 回收站模式：只显示已删除的项目
        return item->isDeleted();
    } else {
        // 正常模式：只显示未删除的项目
        bool statusMatch = !item->isDeleted();

        // 在未删除的项目中进一步筛选完成状态
        if (!m_currentFilter.isEmpty()) {
            if (m_currentFilter == "done") {
                statusMatch = statusMatch && item->isCompleted();
            } else if (m_currentFilter == "todo") {
                statusMatch = statusMatch && !item->isCompleted();
            }
        }

        return statusMatch;
    }
}

/**
 * @brief 检查日期筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoFilter::checkDateMatch(const TodoItem *item) const {
    // 日期筛选逻辑：
    // 如果启用了日期筛选，检查任务的截止日期是否在指定范围内
    if (!m_dateFilterEnabled) {
        return true; // 未启用日期筛选，显示所有
    }

    if (!item->deadline().isValid()) {
        // 如果启用了日期筛选但任务没有截止日期，则不显示
        return false;
    }

    QDate itemDate = item->deadline().date();
    bool startMatch = !m_dateFilterStart.isValid() || itemDate >= m_dateFilterStart;
    bool endMatch = !m_dateFilterEnd.isValid() || itemDate <= m_dateFilterEnd;

    return startMatch && endMatch;
}

/**
 * @brief 发射筛选条件变化信号
 */
void TodoFilter::emitFiltersChanged() {
    emit filtersChanged();
}