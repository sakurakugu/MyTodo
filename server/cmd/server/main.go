package main

import (
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/gorilla/mux"

	"MyTodo/internal/config"
	"MyTodo/internal/handlers"
	"MyTodo/internal/middleware"
)

func main() {
	// 加载配置
	cfg, err := config.LoadConfig()
	if err != nil {
		log.Fatalf("加载配置失败: %v", err)
	}

	// 设置优雅关闭
	defer cfg.CloseDatabase()

	// 创建路由器
	router := mux.NewRouter()

	// 设置CORS中间件
	corsMiddleware := middleware.CORSMiddleware()

	// 创建处理器
	authHandler := handlers.NewAuthHandler()
	todoHandler := handlers.NewTodoHandler()
	categoryHandler := handlers.NewCategoryHandler()

	// 创建认证中间件
	authMiddleware := middleware.NewAuthMiddleware()

	// 设置路由

	// 不需要认证的路由
	router.HandleFunc("/api/v1/auth", authHandler.HandleAuth).Methods("POST", "OPTIONS")
	router.HandleFunc("/api/v1/health", todoHandler.HealthCheck).Methods("GET", "OPTIONS")

	// 需要认证的路由
	api := router.PathPrefix("/api/v1").Subrouter()
	api.Use(authMiddleware.Authenticate)

	// 待办事项路由
	api.HandleFunc("/todos", todoHandler.HandleTodos).Methods("GET", "POST")
	api.HandleFunc("/todos/batch", todoHandler.HandleTodos).Methods("POST")
	api.HandleFunc("/todos/check_new", todoHandler.HandleTodos).Methods("GET")
	api.HandleFunc("/todos/{id:[0-9]+}", todoHandler.HandleTodos).Methods("GET", "PUT", "PATCH", "DELETE")

	// 分类路由
	api.HandleFunc("/categories", categoryHandler.HandleCategories).Methods("GET", "POST", "PUT", "DELETE")

	// 应用CORS中间件到整个路由器
	handler := corsMiddleware.Handler(router)

	// 启动服务器
	port := cfg.App.Port
	if port == "" {
		port = "8080"
	}

	server := &http.Server{
		Addr:    ":" + port,
		Handler: handler,
	}

	// 启动服务器的goroutine
	go func() {
		log.Printf("服务器启动在端口 %s", port)
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("服务器启动失败: %v", err)
		}
	}()

	// 等待中断信号
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	log.Println("服务器正在关闭...")

	// 优雅关闭服务器
	if err := server.Close(); err != nil {
		log.Printf("服务器关闭时出错: %v", err)
	}

	log.Println("服务器已关闭")
}
