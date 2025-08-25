/**
 * @file todo_sorter.h
 * @brief TodoSorter类的头文件
 *
 * 该文件定义了TodoSorter类，用于管理待办事项的排序功能。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include "items/todo_item.h"

/**
 * @class TodoSorter
 * @brief 待办事项排序器，负责管理所有排序逻辑
 *
 * TodoSorter类专门负责待办事项的排序功能，包括：
 *
 * **排序功能：**
 * - 按创建时间排序
 * - 按截止日期排序
 * - 按重要程度排序
 * - 按标题排序
 *
 * **架构特点：**
 * - 继承自QObject，支持Qt信号槽机制
 * - 提供多种排序算法
 * - 支持自定义排序规则
 *
 * @note 该类是线程安全的
 * @see TodoItem, TodoManager
 */
class TodoSorter : public QObject {
    Q_OBJECT
    Q_PROPERTY(int sortType READ sortType WRITE setSortType NOTIFY sortTypeChanged)

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

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TodoSorter(QObject *parent = nullptr);
    ~TodoSorter();

    // 排序属性访问器
    int sortType() const;       // 获取当前排序类型
    void setSortType(int type); // 设置排序类型

    void sortTodos(std::vector<std::unique_ptr<TodoItem>> &todos) const; // 对待办事项列表进行排序
    void sortTodoPointers(QList<TodoItem *> &todos) const; // 对待办事项指针列表进行排序
    static QString getSortTypeName(SortType type); // 获取排序类型的显示名称
    static QList<SortType> getAvailableSortTypes(); // 获取所有可用的排序类型

signals:
    void sortTypeChanged(); // 排序类型变化信号

private:
    int m_sortType; ///< 当前排序类型
};