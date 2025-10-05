package database

import (
	"database/sql"
	"fmt"
	"strings"
	"time"

	"MyTodo/internal/config"
	"MyTodo/internal/models"
)

// TodoRepository 待办事项仓库
type TodoRepository struct {
	db *sql.DB
}

// NewTodoRepository 创建待办事项仓库
func NewTodoRepository() *TodoRepository {
	return &TodoRepository{
		db: config.GlobalConfig.Database.DB,
	}
}

// CreateTodo 创建待办事项
func (tr *TodoRepository) CreateTodo(userUUID string, req *models.CreateTodoRequest) (*models.Todo, error) {
	// 强制要求客户端生成 UUID
	if strings.TrimSpace(req.UUID) == "" {
		return nil, fmt.Errorf("uuid 不能为空，需由客户端生成")
	}
	uuid := strings.TrimSpace(req.UUID)

	query := `
		INSERT INTO todos (uuid, title, description, category, important, is_completed, deadline, 
			recurrence_interval, recurrence_count, recurrence_start_date, user_uuid, , is_trashed, trashed_at, created_at, updated_at)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NOW(), NOW())
	`

	_, err := tr.db.Exec(query, uuid, req.Title, req.Description, req.Category, req.Important,
		req.IsCompleted, req.Deadline, req.RecurrenceInterval, req.RecurrenceCount,
		req.RecurrenceStartDate, userUUID, req.IsTrashed, req.TrashedAt,
	)
	if err != nil {
		return nil, err
	}

	return tr.GetTodoByUUID(uuid, userUUID)
}

// GetTodoByUUID 根据UUID获取待办事项
func (tr *TodoRepository) GetTodoByUUID(uuid, userUUID string) (*models.Todo, error) {
	query := `
		SELECT uuid, title, description, category, important, is_completed, deadline,
			recurrence_interval, recurrence_count, recurrence_start_date, user_uuid,
			created_at, updated_at, is_trashed, trashed_at
		FROM todos 
		WHERE uuid = ? AND user_uuid = ? AND is_trashed = FALSE
	`

	var todo models.Todo
	err := tr.db.QueryRow(query, uuid, userUUID).Scan(
		&todo.UUID, &todo.Title, &todo.Description, &todo.Category,
		&todo.Important, &todo.IsCompleted, &todo.Deadline, &todo.RecurrenceInterval,
		&todo.RecurrenceCount, &todo.RecurrenceStartDate, &todo.UserUUID,
		&todo.CreatedAt, &todo.UpdatedAt, &todo.IsTrashed, &todo.TrashedAt,
	)

	if err != nil {
		return nil, err
	}

	return &todo, nil
}

// GetTodos 获取待办事项列表
func (tr *TodoRepository) GetTodos(userUUID string, filters map[string]interface{}) ([]*models.Todo, error) {
	var conditions []string
	var args []interface{}

	// 基本条件
	conditions = append(conditions, "user_uuid = ?")
	args = append(args, userUUID)
	conditions = append(conditions, "is_trashed = FALSE")

	// 添加筛选条件
	if completed, ok := filters["is_completed"].(bool); ok {
		conditions = append(conditions, "is_completed = ?")
		args = append(args, completed)
	}

	if important, ok := filters["important"].(bool); ok {
		conditions = append(conditions, "important = ?")
		args = append(args, important)
	}

	if categories, ok := filters["category"].([]string); ok && len(categories) > 0 {
		placeholders := strings.Repeat("?,", len(categories))
		placeholders = placeholders[:len(placeholders)-1] // 移除最后的逗号
		conditions = append(conditions, fmt.Sprintf("category IN (%s)", placeholders))
		for _, cat := range categories {
			args = append(args, cat)
		}
	}

	if search, ok := filters["search"].(string); ok && search != "" {
		conditions = append(conditions, "(title LIKE ? OR description LIKE ?)")
		searchTerm := "%" + search + "%"
		args = append(args, searchTerm, searchTerm)
	}

	if deadlineBefore, ok := filters["deadline_before"].(time.Time); ok {
		conditions = append(conditions, "deadline <= ?")
		args = append(args, deadlineBefore)
	}

	if deadlineAfter, ok := filters["deadline_after"].(time.Time); ok {
		conditions = append(conditions, "deadline >= ?")
		args = append(args, deadlineAfter)
	}

	// 构建查询
	query := `
		SELECT uuid, title, description, category, important, is_completed, deadline,
			recurrence_interval, recurrence_count, recurrence_start_date, user_uuid,
			created_at, updated_at, is_trashed, trashed_at
		FROM todos 
		WHERE ` + strings.Join(conditions, " AND ")

	// 添加排序
	if sort, ok := filters["sort"].(string); ok && sort != "" {
		direction := "ASC"
		if dir, ok := filters["direction"].(string); ok && strings.ToUpper(dir) == "DESC" {
			direction = "DESC"
		}
		query += fmt.Sprintf(" ORDER BY %s %s", sort, direction)
	} else {
		query += " ORDER BY created_at DESC"
	}

	// 添加分页
	if limit, ok := filters["limit"].(int); ok && limit > 0 {
		query += " LIMIT ?"
		args = append(args, limit)

		if offset, ok := filters["offset"].(int); ok && offset > 0 {
			query += " OFFSET ?"
			args = append(args, offset)
		}
	}

	rows, err := tr.db.Query(query, args...)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var todos []*models.Todo
	for rows.Next() {
		var todo models.Todo
		err := rows.Scan(
			&todo.UUID, &todo.Title, &todo.Description, &todo.Category,
			&todo.Important, &todo.IsCompleted, &todo.Deadline, &todo.RecurrenceInterval,
			&todo.RecurrenceCount, &todo.RecurrenceStartDate, &todo.UserUUID,
			&todo.CreatedAt, &todo.UpdatedAt, &todo.IsTrashed, &todo.TrashedAt,
		)
		if err != nil {
			return nil, err
		}
		todos = append(todos, &todo)
	}

	return todos, nil
}

// UpdateTodo 更新待办事项
func (tr *TodoRepository) UpdateTodo(uuid, userUUID string, req *models.UpdateTodoRequest) error {
	var setParts []string
	var args []interface{}

	if req.Title != nil {
		setParts = append(setParts, "title = ?")
		args = append(args, *req.Title)
	}
	if req.Description != nil {
		setParts = append(setParts, "description = ?")
		args = append(args, *req.Description)
	}
	if req.Category != nil {
		setParts = append(setParts, "category = ?")
		args = append(args, *req.Category)
	}
	if req.Important != nil {
		setParts = append(setParts, "important = ?")
		args = append(args, *req.Important)
	}
	if req.IsCompleted != nil {
		setParts = append(setParts, "is_completed = ?")
		args = append(args, *req.IsCompleted)
	}
	if req.Deadline != nil {
		setParts = append(setParts, "deadline = ?")
		args = append(args, *req.Deadline)
	}
	if req.RecurrenceInterval != nil {
		setParts = append(setParts, "recurrence_interval = ?")
		args = append(args, *req.RecurrenceInterval)
	}
	if req.RecurrenceCount != nil {
		setParts = append(setParts, "recurrence_count = ?")
		args = append(args, *req.RecurrenceCount)
	}
	if req.RecurrenceStartDate != nil {
		setParts = append(setParts, "recurrence_start_date = ?")
		args = append(args, *req.RecurrenceStartDate)
	}
	if req.IsTrashed != nil {
		setParts = append(setParts, "is_trashed = ?")
		args = append(args, req.IsTrashed)
	}
	if req.TrashedAt != nil {
		setParts = append(setParts, "trashed_at = ?")
		args = append(args, req.TrashedAt)
	}

	if len(setParts) == 0 {
		return fmt.Errorf("没有字段需要更新")
	}

	// 更新待办事项
	setParts = append(setParts, "updated_at = NOW()")
	args = append(args, uuid, userUUID)

	query := fmt.Sprintf(`
		UPDATE todos 
		SET %s 
		WHERE uuid = ? AND user_uuid = ? AND is_trashed = FALSE
	`, strings.Join(setParts, ", "))

	result, err := tr.db.Exec(query, args...)
	if err != nil {
		return err
	}

	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return fmt.Errorf("待办事项不存在或无权限修改")
	}

	return nil
}

// SoftDeleteTodo 软删除待办事项
func (tr *TodoRepository) SoftDeleteTodo(uuid, userUUID string) error {
	query := `
		UPDATE todos 
		SET is_trashed = TRUE, trashed_at = NOW(), updated_at = NOW() 
		WHERE uuid = ? AND user_uuid = ? AND is_trashed = FALSE
	`

	result, err := tr.db.Exec(query, uuid, userUUID)
	if err != nil {
		return err
	}

	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return fmt.Errorf("待办事项不存在或无权限删除")
	}

	return nil
}

// HardDeleteTodo 物理删除待办事项
func (tr *TodoRepository) HardDeleteTodo(uuid, userUUID string) error {
	query := `DELETE FROM todos WHERE uuid = ? AND user_uuid = ?`

	result, err := tr.db.Exec(query, uuid, userUUID)
	if err != nil {
		return err
	}

	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return fmt.Errorf("待办事项不存在或无权限删除")
	}

	return nil
}

// BatchOperation 批量操作
func (tr *TodoRepository) BatchOperation(userUUID string, req *models.BatchOperationRequest) error {
	if len(req.UUIDs) == 0 {
		return fmt.Errorf("没有提供要操作的UUID")
	}

	switch req.Action {
	case "delete", "soft_delete":
		return tr.batchSoftDelete(userUUID, req.UUIDs)
	case "hard_delete":
		return tr.batchHardDelete(userUUID, req.UUIDs)
	case "mark_done":
		return tr.batchUpdateCompleted(userUUID, req.UUIDs, true)
	case "mark_todo":
		return tr.batchUpdateCompleted(userUUID, req.UUIDs, false)
	case "change_category":
		if req.Category == nil {
			return fmt.Errorf("批量更改分类需要提供分类名称")
		}
		return tr.batchUpdateCategory(userUUID, req.UUIDs, *req.Category)
	case "change_important":
		if req.Important == nil {
			return fmt.Errorf("批量更改重要性需要提供重要性值")
		}
		return tr.batchUpdateImportant(userUUID, req.UUIDs, *req.Important)
	default:
		return fmt.Errorf("不支持的批量操作: %s", req.Action)
	}
}

func (tr *TodoRepository) batchSoftDelete(userUUID string, uuids []string) error {
	placeholders := strings.Repeat("?,", len(uuids))
	placeholders = placeholders[:len(placeholders)-1]

	query := fmt.Sprintf(`
		UPDATE todos 
		SET is_trashed = TRUE, trashed_at = NOW(), updated_at = NOW()
		WHERE user_uuid = ? AND uuid IN (%s) AND is_trashed = FALSE
	`, placeholders)

	args := []interface{}{userUUID}
	for _, uuid := range uuids {
		args = append(args, uuid)
	}

	_, err := tr.db.Exec(query, args...)
	return err
}

func (tr *TodoRepository) batchHardDelete(userUUID string, uuids []string) error {
	placeholders := strings.Repeat("?,", len(uuids))
	placeholders = placeholders[:len(placeholders)-1]

	query := fmt.Sprintf(`DELETE FROM todos WHERE user_uuid = ? AND uuid IN (%s)`, placeholders)

	args := []interface{}{userUUID}
	for _, uuid := range uuids {
		args = append(args, uuid)
	}

	_, err := tr.db.Exec(query, args...)
	return err
}

func (tr *TodoRepository) batchUpdateCompleted(userUUID string, uuids []string, completed bool) error {
	placeholders := strings.Repeat("?,", len(uuids))
	placeholders = placeholders[:len(placeholders)-1]

	query := fmt.Sprintf(`
		UPDATE todos 
		SET is_completed = ?, updated_at = NOW()
		WHERE user_uuid = ? AND uuid IN (%s) AND is_trashed = FALSE
	`, placeholders)

	args := []interface{}{completed, userUUID}
	for _, uuid := range uuids {
		args = append(args, uuid)
	}

	_, err := tr.db.Exec(query, args...)
	return err
}

func (tr *TodoRepository) batchUpdateCategory(userUUID string, uuids []string, category string) error {
	placeholders := strings.Repeat("?,", len(uuids))
	placeholders = placeholders[:len(placeholders)-1]

	query := fmt.Sprintf(`
		UPDATE todos 
		SET category = ?, updated_at = NOW()
		WHERE user_uuid = ? AND uuid IN (%s) AND is_trashed = FALSE
	`, placeholders)

	args := []interface{}{category, userUUID}
	for _, uuid := range uuids {
		args = append(args, uuid)
	}

	_, err := tr.db.Exec(query, args...)
	return err
}

func (tr *TodoRepository) batchUpdateImportant(userUUID string, uuids []string, important bool) error {
	placeholders := strings.Repeat("?,", len(uuids))
	placeholders = placeholders[:len(placeholders)-1]

	query := fmt.Sprintf(`
		UPDATE todos 
		SET important = ?, updated_at = NOW()
		WHERE user_uuid = ? AND uuid IN (%s) AND is_trashed = FALSE
	`, placeholders)

	args := []interface{}{important, userUUID}
	for _, uuid := range uuids {
		args = append(args, uuid)
	}

	_, err := tr.db.Exec(query, args...)
	return err
}
