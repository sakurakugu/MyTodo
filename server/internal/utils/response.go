package utils

import (
	"encoding/json"
	"net/http"

	"MyTodo/internal/models"
)

// ResponseHelper 响应助手
type ResponseHelper struct{}

// NewResponseHelper 创建新的响应助手
func NewResponseHelper() *ResponseHelper {
	return &ResponseHelper{}
}

// Success 成功响应
func (r *ResponseHelper) Success(w http.ResponseWriter, data interface{}, message string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)

	response := models.APIResponse{
		Success: true,
		Message: message,
		Data:    data,
	}

	json.NewEncoder(w).Encode(response)
}

// Error 错误响应
func (r *ResponseHelper) Error(w http.ResponseWriter, statusCode int, code, message, details string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(statusCode)

	response := models.APIResponse{
		Success: false,
		Message: message,
		Error: &models.APIError{
			Code:    code,
			Message: message,
			Details: details,
		},
	}

	json.NewEncoder(w).Encode(response)
}

// ValidationError 验证错误
func (r *ResponseHelper) ValidationError(w http.ResponseWriter, message string) {
	r.Error(w, http.StatusBadRequest, "VALIDATION_ERROR", message, "")
}

// NotFoundError 404错误
func (r *ResponseHelper) NotFoundError(w http.ResponseWriter, message string) {
	r.Error(w, http.StatusNotFound, "NOT_FOUND", message, "")
}

// UnauthorizedError 401错误
func (r *ResponseHelper) UnauthorizedError(w http.ResponseWriter, message string, code ...string) {
	errCode := "UNAUTHORIZED"
	if len(code) > 0 {
		errCode = code[0]
	}
	r.Error(w, http.StatusUnauthorized, errCode, message, "")
}

// ForbiddenError 403错误
func (r *ResponseHelper) ForbiddenError(w http.ResponseWriter, message string) {
	r.Error(w, http.StatusForbidden, "FORBIDDEN", message, "")
}

// MethodNotAllowedError 405错误
func (r *ResponseHelper) MethodNotAllowedError(w http.ResponseWriter, allowedMethods []string) {
	w.Header().Set("Allow", joinStrings(allowedMethods, ", "))
	r.Error(w, http.StatusMethodNotAllowed, "METHOD_NOT_ALLOWED", "不支持的请求方法", "")
}

// ServerError 500错误
func (r *ResponseHelper) ServerError(w http.ResponseWriter, message string) {
	r.Error(w, http.StatusInternalServerError, "INTERNAL_SERVER_ERROR", message, "")
}

// DatabaseError 数据库错误
func (r *ResponseHelper) DatabaseError(w http.ResponseWriter) {
	r.Error(w, http.StatusInternalServerError, "DATABASE_ERROR", "数据库连接失败", "")
}

// joinStrings 连接字符串
func joinStrings(strings []string, separator string) string {
	if len(strings) == 0 {
		return ""
	}

	result := strings[0]
	for i := 1; i < len(strings); i++ {
		result += separator + strings[i]
	}

	return result
}

// 全局响应助手实例
var ResponseHelperInstance = NewResponseHelper()
