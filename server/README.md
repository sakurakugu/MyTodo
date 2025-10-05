# Todo App - Go版本

这是从PHP转换而来的Go版本的待办事项管理API。

## 项目结构

```
├── cmd/server/         # 主应用入口
├── internal/
│   ├── config/        # 配置管理
│   ├── database/      # 数据库操作层
│   ├── handlers/      # HTTP处理器
│   ├── middleware/    # 中间件
│   ├── models/        # 数据模型
│   └── utils/         # 工具函数
├── .env               # 环境配置
├── Dockerfile         # Docker构建文件
├── build.sh           # 构建脚本
└── go.mod             # Go模块定义
```

## 功能特性

### ✅ 已完成功能
- **JWT身份认证系统**
  - 用户注册、登录
  - 访问令牌和刷新令牌
  - 令牌验证中间件

- **待办事项管理**
  - CRUD操作（创建、读取、更新、删除）
  - 分页、筛选、排序
  - 批量操作（标记完成、删除、更改分类等）
  - 软删除和硬删除
  - 重复任务支持

- **分类管理**
  - 创建、更新、删除分类
  - 分类关联的待办事项管理

- **安全特性**
  - CORS支持
  - 密码哈希
  - JWT安全实现
  - 参数验证

## 快速开始

### 1. 环境准备

确保安装了Go 1.21+和MySQL数据库。

### 2. 配置环境

复制并修改`.env`文件：

```bash
cp .env .env.local
# 编辑.env.local文件，修改数据库连接信息
```

### 3. 下载依赖

```bash
go mod download
```

### 4. 构建并运行

```bash
# 使用构建脚本
# chmod +x build.sh # 要先赋予执行权限
./build.sh

# 或直接运行
go run cmd/server/main.go
```

### 5. Docker部署

```bash
# 构建Docker镜像
./build.sh docker

# 运行容器
docker run -p 8080:8080 --env-file .env todo-app:latest
```

## API端点

### 认证相关
- `POST /api/auth?action=register` - 用户注册
- `POST /api/auth?action=login` - 用户登录  
- `POST /api/auth?action=refresh` - 刷新令牌

### 待办事项
- `GET /api/todos` - 获取待办事项列表
- `POST /api/todos` - 创建待办事项
- `PUT /api/todos/{id}` - 更新待办事项
- `PATCH /api/todos/{id}` - 部分更新待办事项
- `DELETE /api/todos/{id}` - 删除待办事项
- `POST /api/todos/batch` - 批量操作
- `GET /api/todos/health` - 健康检查

### 分类管理
- `GET /api/categories` - 获取分类列表
- `POST /api/categories` - 创建分类
- `PUT /api/categories` - 更新分类
- `DELETE /api/categories` - 删除分类

## 数据库表结构

参考 docs/数据库表结构.md

## 从PHP版本迁移

### 主要改进
1. **性能提升** - Go的并发性能远超PHP
2. **类型安全** - 静态类型检查减少运行时错误
3. **内存效率** - 更低的内存占用
4. **部署简单** - 单一二进制文件，无需PHP运行时
5. **更好的错误处理** - 显式错误处理

### API兼容性
- 保持了相同的API设计和响应格式
- 支持相同的认证机制
- 数据库表结构无需修改

## 开发说明

### 代码风格
- 遵循Go官方代码规范
- 使用gofmt格式化代码
- 包和函数有清晰的文档注释

### 错误处理
- 统一的错误响应格式
- 详细的错误日志记录
- 优雅的错误降级

### 安全考虑
- JWT令牌安全实现
- 密码安全哈希
- SQL注入防护
- CORS配置

## 许可证

[在此添加许可证信息]