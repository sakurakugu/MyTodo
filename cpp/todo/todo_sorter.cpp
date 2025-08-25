/**
 * @file todo_sorter.cpp
 * @brief TodoSorter类的实现文件
 *
 * 该文件实现了TodoSorter类的所有排序功能。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#include "todo_sorter.h"
#include <QDebug>
#include <algorithm>
#include <Qt>

TodoSorter::TodoSorter(QObject *parent)
    : QObject(parent),
      m_sortType(SortByCreatedTime) {
}

TodoSorter::~TodoSorter() {
}

/**
 * @brief 获取当前排序类型
 * @return 当前排序类型
 */
int TodoSorter::sortType() const {
    return m_sortType;
}

/**
 * @brief 设置排序类型
 * @param type 排序类型
 */
void TodoSorter::setSortType(int type) {
    if (m_sortType != type) {
        m_sortType = type;
        emit sortTypeChanged();
    }
}

/**
 * @brief 对待办事项列表进行排序
 * @param todos 待排序的待办事项列表
 */
void TodoSorter::sortTodos(std::vector<std::unique_ptr<TodoItem>> &todos) const {
    switch (static_cast<SortType>(m_sortType)) {
    case SortByDeadline:
        std::sort(todos.begin(), todos.end(), [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
            QDateTime deadlineA = a->deadline();
            QDateTime deadlineB = b->deadline();
            if (deadlineA.isValid() && !deadlineB.isValid()) {
                return true;
            }
            if (!deadlineA.isValid() && deadlineB.isValid()) {
                return false;
            }
            if (!deadlineA.isValid() && !deadlineB.isValid()) {
                return a->createdAt() > b->createdAt();
            }
            return deadlineA < deadlineB;
        });
        break;
    case SortByImportance:
        std::sort(todos.begin(), todos.end(), [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
            if (a->important() != b->important()) {
                return a->important() > b->important();
            }
            return a->createdAt() > b->createdAt();
        });
        break;
    case SortByTitle:
        std::sort(todos.begin(), todos.end(), [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
            return a->title().compare(b->title(), Qt::CaseInsensitive) < 0;
        });
        break;
    case SortByCreatedTime:
    default:
        std::sort(todos.begin(), todos.end(), [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
            return a->createdAt() > b->createdAt();
        });
        break;
    }
}

/**
 * @brief 对待办事项指针列表进行排序
 * @param todos 待排序的待办事项指针列表
 */
void TodoSorter::sortTodoPointers(QList<TodoItem *> &todos) const {
    switch (static_cast<SortType>(m_sortType)) {
    case SortByDeadline:
        std::sort(todos.begin(), todos.end(), [](const TodoItem *a, const TodoItem *b) {
            QDateTime deadlineA = a->deadline();
            QDateTime deadlineB = b->deadline();
            if (deadlineA.isValid() && !deadlineB.isValid()) {
                return true;
            }
            if (!deadlineA.isValid() && deadlineB.isValid()) {
                return false;
            }
            if (!deadlineA.isValid() && !deadlineB.isValid()) {
                return a->createdAt() > b->createdAt();
            }
            return deadlineA < deadlineB;
        });
        break;
    case SortByImportance:
        std::sort(todos.begin(), todos.end(), [](const TodoItem *a, const TodoItem *b) {
            if (a->important() != b->important()) {
                return a->important() > b->important();
            }
            return a->createdAt() > b->createdAt();
        });
        break;
    case SortByTitle:
        std::sort(todos.begin(), todos.end(), [](const TodoItem *a, const TodoItem *b) {
            return a->title().compare(b->title(), Qt::CaseInsensitive) < 0;
        });
        break;
    case SortByCreatedTime:
    default:
        std::sort(todos.begin(), todos.end(), [](const TodoItem *a, const TodoItem *b) {
            return a->createdAt() > b->createdAt();
        });
        break;
    }
}

/**
 * @brief 获取排序类型的显示名称
 * @param type 排序类型
 * @return 排序类型的显示名称
 */
QString TodoSorter::getSortTypeName(SortType type) {
    switch (type) {
    case SortByCreatedTime:
        return QObject::tr("按创建时间");
    case SortByDeadline:
        return QObject::tr("按截止日期");
    case SortByImportance:
        return QObject::tr("按重要程度");
    case SortByTitle:
        return QObject::tr("按标题");
    default:
        return QObject::tr("未知排序");
    }
}

/**
 * @brief 获取所有可用的排序类型
 * @return 排序类型列表
 */
QList<TodoSorter::SortType> TodoSorter::getAvailableSortTypes() {
    return {SortByCreatedTime, SortByDeadline, SortByImportance, SortByTitle};
}

// 排序比较函数实现已内联到lambda表达式中