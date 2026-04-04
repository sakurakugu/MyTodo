package middleware

import (
	"net/http"

	"github.com/rs/cors"
)

// CORSMiddleware 创建CORS中间件
func CORSMiddleware() *cors.Cors {
	return cors.New(cors.Options{
		AllowedOrigins: []string{
			"https://api.sakurakugu.top",
			"https://sakurakugu.top",
			"http://localhost",
			"https://localhost",
			"*", // 开发阶段允许所有来源
		},
		AllowedMethods: []string{
			http.MethodGet,
			http.MethodPost,
			http.MethodPut,
			http.MethodDelete,
			http.MethodOptions,
		},
		AllowedHeaders: []string{
			"Content-Type",
			"Authorization",
			"X-Requested-With",
		},
		AllowCredentials: true,
	})
}
