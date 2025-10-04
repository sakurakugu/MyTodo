package database

import (
	"database/sql"
	"fmt"
	"strings"

	"MyTodo/internal/config"
	"MyTodo/internal/models"
	"MyTodo/internal/utils"
)

// CategoryRepository 分类仓库
type CategoryRepository struct {
	db *sql.DB
}

// NewCategoryRepository 创建分类仓库
func NewCategoryRepository() *CategoryRepository {
	return &CategoryRepository{
		db: config.GlobalConfig.Database.DB,
	}
}

// CreateCategory 使用客户端提供的 uuid 创建分类。
// 若名称已存在则返回已存在错误以便上层判定忽略；若 uuid 已存在则同样报错避免冲突。
func (cr *CategoryRepository) CreateCategory(userUUID, uuidStr, name string) (*models.Category, error) {
	cleaned := standardizeName(name)
	if cleaned == "" {
		return nil, fmt.Errorf("分类名称不能为空")
	}

	// 名称唯一性（同一用户）
	exists, err := cr.CategoryExistsByName(userUUID, cleaned)
	if err != nil {
		return nil, err
	}
	if exists {
		return nil, fmt.Errorf("分类名称已存在")
	}

	uuidToUse := uuidStr
	if strings.TrimSpace(uuidToUse) == "" {
		uuidToUse = utils.GenerateUUID()
	}

	// 直接尝试插入（uuid 唯一约束由 DB 保证）
	query := `
		INSERT INTO categories (uuid, name, user_uuid, created_at, updated_at) 
		VALUES (?, ?, ?, NOW(), NOW())
	`
	if _, err := cr.db.Exec(query, uuidToUse, cleaned, userUUID); err != nil {
		return nil, err
	}

	return cr.GetCategoryByUUID(uuidToUse, userUUID)
}

// GetCategoryByUUID 根据UUID获取分类
func (cr *CategoryRepository) GetCategoryByUUID(uuid, userUUID string) (*models.Category, error) {
	query := `
		SELECT uuid, name, user_uuid, created_at, updated_at 
		FROM categories 
		WHERE uuid = ? AND user_uuid = ?
	`

	var category models.Category
	err := cr.db.QueryRow(query, uuid, userUUID).Scan(
		&category.UUID, &category.Name, &category.UserUUID,
		&category.CreatedAt, &category.UpdatedAt,
	)

	if err != nil {
		return nil, err
	}

	return &category, nil
}

// GetAllCategories 获取用户的所有分类
func (cr *CategoryRepository) GetAllCategories(userUUID string) ([]*models.Category, error) {
	query := `
		SELECT uuid, name, user_uuid, created_at, updated_at 
		FROM categories 
		WHERE user_uuid = ?
		ORDER BY name ASC
	`

	rows, err := cr.db.Query(query, userUUID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var categories []*models.Category
	for rows.Next() {
		var category models.Category
		err := rows.Scan(
			&category.UUID, &category.Name, &category.UserUUID,
			&category.CreatedAt, &category.UpdatedAt,
		)
		if err != nil {
			return nil, err
		}
		categories = append(categories, &category)
	}

	return categories, nil
}

// UpdateCategory 更新分类
func (cr *CategoryRepository) UpdateCategory(userUUID string, req *models.UpdateCategoryRequest) error {
	// 必须提供UUID
	if req.UUID == nil || *req.UUID == "" {
		return fmt.Errorf("必须提供分类UUID")
	}

	// 读取旧分类
	category, err := cr.GetCategoryByUUID(*req.UUID, userUUID)
	if err != nil {
		return fmt.Errorf("分类不存在或无权限修改")
	}

	// 标准化名称
	cleaned := standardizeName(req.Name)
	if cleaned == "" {
		return fmt.Errorf("分类名称不能为空")
	}
	req.Name = cleaned

	// 如果名称没有变化直接返回
	if req.Name == category.Name {
		return nil
	}

	// 检查新名称是否已被其他分类使用
	exists, err := cr.CategoryExistsByNameExcludeUUID(userUUID, req.Name, *req.UUID)
	if err != nil {
		return err
	}
	if exists {
		return fmt.Errorf("分类名称已存在")
	}

	// 更新分类
	query := `
		UPDATE categories 
		SET name = ?, updated_at = NOW() 
		WHERE uuid = ? AND user_uuid = ?
	`

	result, err := cr.db.Exec(query, req.Name, *req.UUID, userUUID)
	if err != nil {
		return err
	}

	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return fmt.Errorf("分类不存在或无权限修改")
	}

	return nil
}

// standardizeName 统一分类名称：Trim + 压缩多余空白
func standardizeName(name string) string {
	trimmed := strings.TrimSpace(name)
	if trimmed == "" {
		return ""
	}
	// 将内部连续空白压缩为单个空格
	fields := strings.Fields(trimmed)
	return strings.Join(fields, " ")
}

// DeleteCategory 根据UUID删除分类
func (cr *CategoryRepository) DeleteCategory(uuid, userUUID string) error {
	// 检查分类是否存在
	category, err := cr.GetCategoryByUUID(uuid, userUUID)
	if err != nil {
		return fmt.Errorf("分类不存在或无权限删除")
	}

	// 检查是否为默认分类或"未分类"
	if category.Name == "未分类" {
		return fmt.Errorf("默认分类 \"未分类\" 不能删除")
	}

	// 开始事务
	tx, err := cr.db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()

	// 将使用该分类的待办事项重新分配到"未分类"
	updateTodosQuery := `
		UPDATE todos 
		SET category = "未分类" 
		WHERE category = ? AND user_uuid = ?
	`
	_, err = tx.Exec(updateTodosQuery, category.Name, userUUID)
	if err != nil {
		return err
	}

	// 删除分类
	deleteCategoryQuery := `DELETE FROM categories WHERE uuid = ? AND user_uuid = ?`
	result, err := tx.Exec(deleteCategoryQuery, uuid, userUUID)
	if err != nil {
		return err
	}

	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return fmt.Errorf("分类不存在或无权限删除")
	}

	// 提交事务
	return tx.Commit()
}

// CategoryExistsByName 检查分类名称是否存在
func (cr *CategoryRepository) CategoryExistsByName(userUUID, name string) (bool, error) {
	query := `SELECT COUNT(*) FROM categories WHERE name = ? AND user_uuid = ?`

	var count int
	err := cr.db.QueryRow(query, name, userUUID).Scan(&count)
	if err != nil {
		return false, err
	}

	return count > 0, nil
}

// CategoryExistsByNameExcludeUUID 检查分类名称是否已存在（排除指定UUID）
func (cr *CategoryRepository) CategoryExistsByNameExcludeUUID(userUUID, name string, excludeUUID string) (bool, error) {
	query := `
		SELECT COUNT(*) 
		FROM categories 
		WHERE user_uuid = ? AND name = ? AND uuid != ?
	`

	var count int
	err := cr.db.QueryRow(query, userUUID, name, excludeUUID).Scan(&count)
	if err != nil {
		return false, err
	}

	return count > 0, nil
}
