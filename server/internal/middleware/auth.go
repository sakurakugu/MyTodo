package middleware

import (
	"context"
	"net/http"
	"strings"
	"time"

	"MyTodo/internal/config"
	"MyTodo/internal/utils"
)

// AuthMiddleware 认证中间件
type AuthMiddleware struct {
	jwtHelper *utils.JWTHelper
	response  *utils.ResponseHelper
}

// NewAuthMiddleware 创建新的认证中间件
func NewAuthMiddleware() *AuthMiddleware {
	jwtHelper := utils.NewJWTHelper(
		config.GlobalConfig.Security.JWTSecret,
		60*time.Second,
	)

	return &AuthMiddleware{
		jwtHelper: jwtHelper,
		response:  utils.ResponseHelperInstance,
	}
}

// Authenticate 认证中间件函数
func (a *AuthMiddleware) Authenticate(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// 获取令牌
		token := a.extractToken(r)
		if token == "" {
			a.response.UnauthorizedError(w, "缺少访问令牌")
			return
		}

		// 验证令牌
		claims, err := a.jwtHelper.ValidateToken(token)
		if err != nil {
			a.response.UnauthorizedError(w, "无效的访问令牌")
			return
		}

		// 将用户信息添加到上下文
		ctx := context.WithValue(r.Context(), "user", claims)
		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

// extractToken 从请求中提取令牌
func (a *AuthMiddleware) extractToken(r *http.Request) string {
	// 1. 从 Authorization 头中获取 Bearer 令牌
	authHeader := r.Header.Get("Authorization")
	if authHeader != "" {
		parts := strings.SplitN(authHeader, " ", 2)
		if len(parts) == 2 && strings.ToLower(parts[0]) == "bearer" {
			return parts[1]
		}
	}

	// 2. 从查询参数中获取令牌
	if token := r.URL.Query().Get("token"); token != "" {
		return token
	}

	return ""
}

// GetUserFromContext 从上下文中获取用户信息
func GetUserFromContext(ctx context.Context) (*utils.JWTClaims, bool) {
	user, ok := ctx.Value("user").(*utils.JWTClaims)
	return user, ok
}
