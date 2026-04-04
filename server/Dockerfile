# 使用官方的Go镜像作为构建环境
FROM golang:1.21-alpine AS builder

# 设置工作目录
WORKDIR /app

# 安装必要的包
RUN apk add --no-cache git

# 复制go mod文件
COPY go.mod go.sum ./

# 下载依赖
RUN go mod download

# 复制源代码
COPY . .

# 构建应用
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o main ./cmd/server

# 使用轻量级镜像作为运行环境
FROM alpine:latest

# 安装ca-certificates（用于HTTPS）和时区数据
RUN apk --no-cache add ca-certificates tzdata

# 设置时区
ENV TZ=Asia/Shanghai

# 创建应用用户
RUN addgroup -S appgroup && adduser -S appuser -G appgroup

# 设置工作目录
WORKDIR /root/

# 从构建阶段复制二进制文件
COPY --from=builder /app/main .

# 复制.env文件（如果需要）
COPY --from=builder /app/.env .

# 更改文件所有者
RUN chown -R appuser:appgroup /root/

# 切换到应用用户
USER appuser

# 暴露端口
EXPOSE 8080

# 运行应用
CMD ["./main"]