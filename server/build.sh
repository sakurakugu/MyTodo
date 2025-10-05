#!/bin/bash

# 构建脚本
set -e

echo "开始构建Go项目..."

# 检查Go是否安装
if ! command -v go &> /dev/null; then
    echo "错误: Go未安装"
    exit 1
fi

# 下载依赖
echo "下载依赖..."
go mod tidy
go mod download

# 运行测试（如果有）
# echo "运行测试..."
# go test ./...

# 构建应用
echo "构建应用..."
CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o bin/MyTodo-Server ./cmd/server

echo "构建完成！二进制文件位于: bin/MyTodo-Server"

systemctl daemon-reload
systemctl enable mytodo
systemctl stop mytodo
systemctl start mytodo

echo "服务已启动！"

# 如果需要Docker构建
if [ "$1" = "docker" ]; then
    echo "构建Docker镜像..."
    docker build -t todo-app:latest .
    echo "Docker镜像构建完成！"
fi