package database

import (
	"database/sql"
	"fmt"

	"MyTodo/internal/config"
	"MyTodo/internal/models"
	"MyTodo/internal/utils"
)

// UserRepository 用户仓库
type UserRepository struct {
	db *sql.DB
}

// NewUserRepository 创建用户仓库
func NewUserRepository() *UserRepository {
	return &UserRepository{
		db: config.GlobalConfig.Database.UserDB,
	}
}

// CreateUser 创建用户
func (ur *UserRepository) CreateUser(username, email, password string) (*models.User, error) {
	// 哈希密码
	hashedPassword, err := utils.HashPassword(password)
	if err != nil {
		return nil, err
	}

	uuid := utils.GenerateUUID()
	query := `
		INSERT INTO users (uuid, username, email, password, created_at) 
		VALUES (?, ?, ?, ?, NOW())
	`

	result, err := ur.db.Exec(query, uuid, username, email, hashedPassword)
	if err != nil {
		return nil, err
	}

	id, err := result.LastInsertId()
	if err != nil {
		return nil, err
	}

	return ur.GetUserByID(int(id))
} // GetUserByUsername 根据用户名获取用户
func (ur *UserRepository) GetUserByUsername(username string) (*models.User, error) {
	query := `
		SELECT id, uuid, username, email, password, created_at, last_login 
		FROM users 
		WHERE username = ?
	`

	var user models.User
	err := ur.db.QueryRow(query, username).Scan(
		&user.ID, &user.UUID, &user.Username, &user.Email,
		&user.PasswordHash, &user.CreatedAt, &user.LastLogin,
	)

	if err != nil {
		return nil, err
	}

	return &user, nil
}

// GetUserByEmail 根据邮箱获取用户
func (ur *UserRepository) GetUserByEmail(email string) (*models.User, error) {
	query := `
		SELECT id, uuid, username, email, password, created_at, last_login 
		FROM users 
		WHERE email = ?
	`

	var user models.User
	err := ur.db.QueryRow(query, email).Scan(
		&user.ID, &user.UUID, &user.Username, &user.Email,
		&user.PasswordHash, &user.CreatedAt, &user.LastLogin,
	)

	if err != nil {
		return nil, err
	}

	return &user, nil
}

// GetUserByID 根据ID获取用户
func (ur *UserRepository) GetUserByID(id int) (*models.User, error) {
	query := `
		SELECT id, uuid, username, email, password, created_at, last_login 
		FROM users 
		WHERE id = ?
	`

	var user models.User
	err := ur.db.QueryRow(query, id).Scan(
		&user.ID, &user.UUID, &user.Username, &user.Email,
		&user.PasswordHash, &user.CreatedAt, &user.LastLogin,
	)

	if err != nil {
		return nil, err
	}

	return &user, nil
}

// GetUserByUUID 根据UUID获取用户
func (ur *UserRepository) GetUserByUUID(uuid string) (*models.User, error) {
	query := `
		SELECT id, uuid, username, email, password, created_at, last_login 
		FROM users 
		WHERE uuid = ?
	`

	var user models.User
	err := ur.db.QueryRow(query, uuid).Scan(
		&user.ID, &user.UUID, &user.Username, &user.Email,
		&user.PasswordHash, &user.CreatedAt, &user.LastLogin,
	)

	if err != nil {
		return nil, err
	}

	return &user, nil
}

// CheckUserExists 检查用户是否存在
func (ur *UserRepository) CheckUserExists(username, email string) (bool, error) {
	query := `
		SELECT COUNT(*) FROM users 
		WHERE username = ? OR email = ?
	`

	var count int
	err := ur.db.QueryRow(query, username, email).Scan(&count)
	if err != nil {
		return false, err
	}

	return count > 0, nil
}

// ValidateUser 验证用户登录
func (ur *UserRepository) ValidateUser(username, password string) (*models.User, error) {
	user, err := ur.GetUserByUsername(username)
	if err != nil {
		// 也尝试用邮箱登录
		user, err = ur.GetUserByEmail(username)
		if err != nil {
			return nil, fmt.Errorf("用户不存在")
		}
	}

	if !utils.CheckPassword(user.PasswordHash, password) {
		return nil, fmt.Errorf("密码错误")
	}

	return user, nil
}
