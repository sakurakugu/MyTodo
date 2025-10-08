package models

import (
	"time"
)

// User 用户模型
type User struct {
	ID           int        `json:"id" db:"id"`
	UUID         string     `json:"uuid" db:"uuid"`
	Username     string     `json:"username" db:"username"`
	Email        string     `json:"email" db:"email"`
	PasswordHash string     `json:"-" db:"password_hash"`
	CreatedAt    time.Time  `json:"created_at" db:"created_at"`
	LastLogin    *time.Time `json:"last_login" db:"last_login"`
}

// Todo 待办事项模型
type Todo struct {
	UUID                string     `json:"uuid" db:"uuid"`
	Title               string     `json:"title" db:"title"`
	Description         *string    `json:"description" db:"description"`
	Category            string     `json:"category" db:"category"`
	Important           bool       `json:"important" db:"important"`
	IsCompleted         bool       `json:"is_completed" db:"is_completed"`
	Deadline            *time.Time `json:"deadline" db:"deadline"`
	RecurrenceInterval  *int       `json:"recurrence_interval" db:"recurrence_interval"`
	RecurrenceCount     *int       `json:"recurrence_count" db:"recurrence_count"`
	RecurrenceStartDate *time.Time `json:"recurrence_start_date" db:"recurrence_start_date"`
	UserUUID            string     `json:"user_uuid" db:"user_uuid"`
	CreatedAt           time.Time  `json:"created_at" db:"created_at"`
	UpdatedAt           time.Time  `json:"updated_at" db:"updated_at"`
	IsTrashed           bool       `json:"is_trashed" db:"is_trashed"`
	TrashedAt           *time.Time `json:"trashed_at" db:"trashed_at"`
}

// Category 分类模型
type Category struct {
	UUID      string    `json:"uuid" db:"uuid"`
	Name      string    `json:"name" db:"name"`
	UserUUID  string    `json:"user_uuid" db:"user_uuid"`
	CreatedAt time.Time `json:"created_at" db:"created_at"`
	UpdatedAt time.Time `json:"updated_at" db:"updated_at"`
}

// LoginRequest 登录请求
type LoginRequest struct {
	Account  string `json:"account" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// RegisterRequest 注册请求
type RegisterRequest struct {
	Username string `json:"username" binding:"required"`
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required,min=6"`
}

// TokenResponse 令牌响应
type TokenResponse struct {
	AccessToken  string    `json:"access_token"`
	RefreshToken string    `json:"refresh_token"`
	ExpiresIn    int       `json:"expires_in"`
	TokenType    string    `json:"token_type"`
	User         *UserInfo `json:"user,omitempty"`
}

// UserInfo 认证后返回给客户端的精简用户信息
type UserInfo struct {
	UUID     string `json:"uuid"`
	Username string `json:"username"`
	Email    string `json:"email"`
}

// RefreshTokenRequest 刷新令牌请求
type RefreshTokenRequest struct {
	RefreshToken string `json:"refresh_token" binding:"required"`
}

// CreateTodoRequest 创建待办事项请求
type CreateTodoRequest struct {
	UUID                string     `json:"uuid"`
	Title               string     `json:"title" binding:"required"`
	Description         *string    `json:"description"`
	Category            string     `json:"category"`
	Important           bool       `json:"important"`
	Deadline            *time.Time `json:"deadline"`
	RecurrenceInterval  *int       `json:"recurrence_interval"`
	RecurrenceCount     *int       `json:"recurrence_count"`
	RecurrenceStartDate *time.Time `json:"recurrence_start_date"`
	IsCompleted         bool       `json:"is_completed"`
	IsTrashed           bool       `json:"is_trashed" db:"is_trashed"`
	TrashedAt           *time.Time `json:"trashed_at" db:"trashed_at"`
}

// UpdateTodoRequest 更新待办事项请求
type UpdateTodoRequest struct {
	UUID                string     `json:"uuid"`
	Title               *string    `json:"title"`
	Description         *string    `json:"description"`
	Category            *string    `json:"category"`
	Important           *bool      `json:"important"`
	Deadline            *time.Time `json:"deadline"`
	RecurrenceInterval  *int       `json:"recurrence_interval"`
	RecurrenceCount     *int       `json:"recurrence_count"`
	RecurrenceStartDate *time.Time `json:"recurrence_start_date"`
	IsCompleted         *bool      `json:"is_completed"`
	IsTrashed           *bool      `json:"is_trashed" db:"is_trashed"`
	TrashedAt           *time.Time `json:"trashed_at" db:"trashed_at"`
}

// CreateCategoryRequest 创建分类请求
type CreateCategoryRequest struct {
	UUID string `json:"uuid" binding:"required"`
	Name string `json:"name" binding:"required"`
}

// UpdateCategoryRequest 更新分类请求
type UpdateCategoryRequest struct {
	UUID *string `json:"uuid"`
	Name string  `json:"name" binding:"required"`
}

// BatchOperationRequest 批量操作请求
type BatchOperationRequest struct {
	Action    string   `json:"action" binding:"required"`
	UUIDs     []string `json:"uuids" binding:"required"`
	Category  *string  `json:"category"`
	Important *bool    `json:"important"`
}

// BatchSyncRequest 批量同步请求（客户端发送的格式）
type BatchSyncRequest struct {
	Todos []SyncTodoItem `json:"todos" binding:"required"`
}

// SyncTodoItem 同步的待办事项项目
type SyncTodoItem struct {
	UUID                string     `json:"uuid" binding:"required"`
	UserUUID            string     `json:"user_uuid" binding:"required"`
	Title               string     `json:"title" binding:"required"`
	Description         *string    `json:"description"`
	Category            string     `json:"category"`
	Important           bool       `json:"important"`
	Deadline            *time.Time `json:"deadline"`
	RecurrenceInterval  *int       `json:"recurrenceInterval"`
	RecurrenceCount     *int       `json:"recurrenceCount"`
	RecurrenceStartDate *string    `json:"recurrenceStartDate"`
	IsCompleted         bool       `json:"is_completed"`
	CompletedAt         *time.Time `json:"completed_at"`
	IsTrashed           bool       `json:"is_trashed"`
	TrashedAt           *time.Time `json:"trashed_at"`
	CreatedAt           *time.Time `json:"created_at"`
	UpdatedAt           *time.Time `json:"updated_at"`
	Synced              *int       `json:"synced"` // 1=新增 2=更新 3=删除 其它/空=自动判断
}

// BatchCategorySyncRequest 批量分类同步请求（客户端 categories 批量推送格式）
type BatchCategorySyncRequest struct {
	Categories []SyncCategoryItem `json:"categories" binding:"required"`
}

// SyncCategoryItem 同步用的分类条目
// Synced 语义（与客户端约定）：
// 1=新增 2=更新 3=删除；为空或其他值时按 upsert 处理
type SyncCategoryItem struct {
	UUID      string     `json:"uuid" binding:"required"`
	Name      string     `json:"name" binding:"required"`
	CreatedAt *time.Time `json:"created_at"`
	UpdatedAt *time.Time `json:"updated_at"`
	Synced    *int       `json:"synced"`
}

// APIResponse 标准API响应格式
type APIResponse struct {
	Success bool        `json:"success"`
	Message string      `json:"message"`
	Data    interface{} `json:"data,omitempty"`
	Error   *APIError   `json:"error,omitempty"`
}

// APIError 错误信息
type APIError struct {
	Code    string `json:"code"`
	Message string `json:"message"`
	Details string `json:"details,omitempty"`
}
