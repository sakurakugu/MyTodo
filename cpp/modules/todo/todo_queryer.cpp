/**
 * @file todo_queryer.cpp
 * @brief TodoQueryer类的实现文件
 *
 * 该文件实现了TodoQueryer类的所有筛选功能。
 *
 * @author Sakurakugu
 * @date 2025-08-25 00:54:11(UTC+8) 周一
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "todo_queryer.h"
#include "todo_item.h"

TodoQueryer::TodoQueryer(QObject *parent)
    : QObject(parent),               //
      m_currentCategory(""),         //
      m_currentFilter(""),           //
      m_searchText(""),              //
      m_dateFilterEnabled(false),    //
      m_sortType(SortByUpdatedTime), //
      m_descending(true)             //
{}

TodoQueryer::~TodoQueryer() {}

/**
 * @brief 获取当前激活的分类筛选器
 * @return 当前分类名称
 */
QString TodoQueryer::currentCategory() const {
    return m_currentCategory;
}

/**
 * @brief 设置分类筛选器
 * @param category 分类名称，空字符串表示显示所有分类
 */
void TodoQueryer::setCurrentCategory(const QString &category) {
    if (m_currentCategory != category) {
        m_currentCategory = category;
        emit currentCategoryChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取当前激活的筛选条件
 * @return 当前筛选条件
 */
QString TodoQueryer::currentFilter() const {
    return m_currentFilter;
}

/**
 * @brief 设置筛选条件
 * @param filter 筛选条件（如"done"、"todo"等）
 */
void TodoQueryer::setCurrentFilter(const QString &filter) {
    if (m_currentFilter != filter) {
        m_currentFilter = filter;
        emit currentFilterChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取搜索文本
 * @return 当前搜索文本
 */
QString TodoQueryer::searchText() const {
    return m_searchText;
}

/**
 * @brief 设置搜索文本
 * @param text 搜索文本
 */
void TodoQueryer::setSearchText(const QString &text) {
    if (m_searchText != text) {
        m_searchText = text;
        emit searchTextChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取日期筛选开始日期
 * @return 日期筛选开始日期
 */
QDate TodoQueryer::dateFilterStart() const {
    return m_dateFilterStart;
}

/**
 * @brief 设置日期筛选开始日期
 * @param date 开始日期
 */
void TodoQueryer::setDateFilterStart(const QDate &date) {
    if (m_dateFilterStart != date) {
        m_dateFilterStart = date;
        emit dateFilterStartChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取日期筛选结束日期
 * @return 日期筛选结束日期
 */
QDate TodoQueryer::dateFilterEnd() const {
    return m_dateFilterEnd;
}

/**
 * @brief 设置日期筛选结束日期
 * @param date 结束日期
 */
void TodoQueryer::setDateFilterEnd(const QDate &date) {
    if (m_dateFilterEnd != date) {
        m_dateFilterEnd = date;
        emit dateFilterEndChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取日期筛选是否启用
 * @return 日期筛选是否启用
 */
bool TodoQueryer::dateFilterEnabled() const {
    return m_dateFilterEnabled;
}

/**
 * @brief 设置日期筛选是否启用
 * @param enabled 是否启用日期筛选
 */
void TodoQueryer::setDateFilterEnabled(bool enabled) {
    if (m_dateFilterEnabled != enabled) {
        m_dateFilterEnabled = enabled;
        emit dateFilterEnabledChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 检查是否有任何筛选条件被激活
 * @return 如果有筛选条件被激活返回true，否则返回false
 */
bool TodoQueryer::是否激活任何查询条件() const {
    return !m_currentCategory.isEmpty() || !m_currentFilter.isEmpty() || !m_searchText.isEmpty() || m_dateFilterEnabled;
}

/**
 * @brief 检查分类筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoQueryer::checkCategoryMatch(const TodoItem *item) const {
    if (m_currentCategory.isEmpty() || m_currentCategory == "全部" || m_currentCategory == "all") {
        return true; // 没有分类筛选，显示全部
    }

    return item->category() == m_currentCategory;
}

/**
 * @brief 检查状态筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoQueryer::checkStatusMatch(const TodoItem *item) const {
    // 状态筛选逻辑：
    // - 如果m_currentFilter为"recycle"，则只显示已删除的项目
    // - 否则只显示未删除的项目，并根据完成状态进一步筛选
    // - 如果m_currentFilter为"all"，则显示所有项目
    if (m_currentFilter == "all") {
        return !item->isTrashed();
    } else if (m_currentFilter == "recycle") {
        // 回收站模式：只显示已删除的项目
        return item->isTrashed();
    } else {
        // 正常模式：只显示未删除的项目
        bool statusMatch = !item->isTrashed();

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
 * @brief 检查搜索文本筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoQueryer::checkSearchMatch(const TodoItem *item) const {
    if (m_searchText.isEmpty()) {
        return true; // 没有搜索文本，显示全部
    }

    // 在标题、描述和分类中搜索
    std::string searchLower = m_searchText.toStdString();
    return item->title().find(searchLower) != std::string::npos ||
           item->description().find(searchLower) != std::string::npos ||
           item->category().find(searchLower) != std::string::npos;
}

/**
 * @brief 检查日期筛选条件
 * @param item 待检查的待办事项
 * @return 如果匹配返回true，否则返回false
 */
bool TodoQueryer::checkDateMatch(const TodoItem *item) const {
    // 日期筛选逻辑：
    // 如果启用了日期筛选，检查任务的截止日期是否在指定范围内
    if (!m_dateFilterEnabled) {
        return true; // 未启用日期筛选，显示所有
    }

    if (!item->deadline().isValid()) {
        // 如果启用了日期筛选但任务没有截止日期，则不显示
        return false;
    }

    QDate itemDate = item->deadline().date().toQDate();
    bool startMatch = !m_dateFilterStart.isValid() || itemDate >= m_dateFilterStart;
    bool endMatch = !m_dateFilterEnd.isValid() || itemDate <= m_dateFilterEnd;

    return startMatch && endMatch;
}

/**
 * @brief 获取当前排序类型
 * @return 当前排序类型
 */
int TodoQueryer::sortType() const {
    return m_sortType;
}

/**
 * @brief 设置排序类型
 * @param type 排序类型
 */
void TodoQueryer::setSortType(int type) {
    if (m_sortType != type) {
        m_sortType = type;
        emit sortTypeChanged();
        emit queryConditionsChanged();
    }
}

/**
 * @brief 获取是否倒序
 * @return 是否倒序
 */
bool TodoQueryer::descending() const {
    return m_descending;
}

/**
 * @brief 设置是否倒序
 * @param desc 是否倒序
 */
void TodoQueryer::setDescending(bool desc) {
    if (m_descending != desc) {
        m_descending = desc;
        emit descendingChanged();
        emit queryConditionsChanged();
    }
}
