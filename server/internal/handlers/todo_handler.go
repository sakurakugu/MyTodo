package handlers

import (
	"encoding/json"
	"io"
	"net/http"
	"strconv"
	"strings"
	"time"

	"MyTodo/internal/database"
	"MyTodo/internal/middleware"
	"MyTodo/internal/models"
	"MyTodo/internal/utils"
)

// TodoHandler 待办事项处理器
type TodoHandler struct {
	todoRepo *database.TodoRepository
	response *utils.ResponseHelper
}

// NewTodoHandler 创建待办事项处理器
func NewTodoHandler() *TodoHandler {
	return &TodoHandler{
		todoRepo: database.NewTodoRepository(),
		response: utils.ResponseHelperInstance,
	}
}

// HandleTodos 统一待办事项处理入口
func (th *TodoHandler) HandleTodos(w http.ResponseWriter, r *http.Request) {
	// 健康检查端点不需要认证
	if strings.HasSuffix(r.URL.Path, "/health") {
		th.HealthCheck(w, r)
		return
	}

	// 获取用户信息
	user, ok := middleware.GetUserFromContext(r.Context())
	if !ok {
		th.response.UnauthorizedError(w, "未授权访问")
		return
	}

	switch r.Method {
	case http.MethodGet:
		// 检查特殊路径
		if strings.HasSuffix(r.URL.Path, "/check_new") {
			th.CheckNewTodos(w, r, user.UserUUID)
		} else {
			th.GetTodos(w, r, user.UserUUID)
		}
	case http.MethodPost:
		// 检查批量操作
		if strings.HasSuffix(r.URL.Path, "/batch") {
			th.BatchOperation(w, r, user.UserUUID)
		} else {
			th.CreateTodo(w, r, user.UserUUID)
		}
	case http.MethodPut:
		th.UpdateTodo(w, r, user.UserUUID)
	case http.MethodPatch:
		th.PatchTodo(w, r, user.UserUUID)
	case http.MethodDelete:
		th.DeleteTodo(w, r, user.UserUUID)
	default:
		th.response.MethodNotAllowedError(w, []string{
			http.MethodGet, http.MethodPost, http.MethodPut,
			http.MethodPatch, http.MethodDelete,
		})
	}
}

// HealthCheck 健康检查
func (th *TodoHandler) HealthCheck(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		th.response.MethodNotAllowedError(w, []string{http.MethodGet})
		return
	}

	th.response.Success(w, map[string]interface{}{
		"status":    "healthy",
		"timestamp": time.Now(),
		"version":   "1.0.0",
	}, "服务正常运行")
}

// GetTodos 获取待办事项列表
func (th *TodoHandler) GetTodos(w http.ResponseWriter, r *http.Request, userUUID string) {
	filters := make(map[string]interface{})

	// 解析查询参数
	query := r.URL.Query()

	// 分页参数
	if page := query.Get("page"); page != "" {
		if pageNum, err := strconv.Atoi(page); err == nil && pageNum > 0 {
			perPage := 20 // 默认每页20条
			if pp := query.Get("per_page"); pp != "" {
				if ppNum, err := strconv.Atoi(pp); err == nil && ppNum > 0 && ppNum <= 100 {
					perPage = ppNum
				}
			}
			filters["limit"] = perPage
			filters["offset"] = (pageNum - 1) * perPage
		}
	}

	// 筛选参数
	if isCompleted := query.Get("is_completed"); isCompleted != "" {
		if completed, err := strconv.ParseBool(isCompleted); err == nil {
			filters["is_completed"] = completed
		}
	}

	if important := query.Get("important"); important != "" {
		if imp, err := strconv.ParseBool(important); err == nil {
			filters["important"] = imp
		}
	}

	if category := query.Get("category"); category != "" {
		categories := strings.Split(category, ",")
		for i, cat := range categories {
			categories[i] = strings.TrimSpace(cat)
		}
		filters["category"] = categories
	}

	if search := query.Get("search"); search != "" {
		filters["search"] = strings.TrimSpace(search)
	}

	// 日期筛选
	if deadlineBefore := query.Get("deadline_before"); deadlineBefore != "" {
		if date, err := time.Parse("2006-01-02", deadlineBefore); err == nil {
			filters["deadline_before"] = date
		}
	}

	if deadlineAfter := query.Get("deadline_after"); deadlineAfter != "" {
		if date, err := time.Parse("2006-01-02", deadlineAfter); err == nil {
			filters["deadline_after"] = date
		}
	}

	// 排序参数
	if sort := query.Get("sort"); sort != "" {
		allowedSorts := []string{"created_at", "updated_at", "title", "category", "important", "deadline", "is_completed"}
		for _, allowed := range allowedSorts {
			if sort == allowed {
				filters["sort"] = sort
				break
			}
		}
	}

	if direction := query.Get("direction"); direction != "" {
		if strings.ToUpper(direction) == "DESC" || strings.ToUpper(direction) == "ASC" {
			filters["direction"] = strings.ToUpper(direction)
		}
	}

	// 时间范围查询
	if sinceTimestamp := query.Get("since_timestamp"); sinceTimestamp != "" {
		if date, err := time.Parse("2006-01-02T15:04:05", sinceTimestamp); err == nil {
			filters["created_at_after"] = date
		}
	}

	if fromDate := query.Get("from_date"); fromDate != "" {
		if date, err := time.Parse("2006-01-02T15:04:05", fromDate); err == nil {
			filters["created_at_after"] = date
		}
	}

	if toDate := query.Get("to_date"); toDate != "" {
		if date, err := time.Parse("2006-01-02T15:04:05", toDate); err == nil {
			filters["created_at_before"] = date
		}
	}

	todos, err := th.todoRepo.GetTodos(userUUID, filters)
	if err != nil {
		th.response.ServerError(w, "获取待办事项失败")
		return
	}

	th.response.Success(w, map[string]interface{}{
		"todos": todos,
		"count": len(todos),
	}, "获取待办事项成功")
}

// CreateTodo 创建待办事项
func (th *TodoHandler) CreateTodo(w http.ResponseWriter, r *http.Request, userUUID string) {
	// 首先尝试解析为批量同步请求
	var batchReq models.BatchSyncRequest
	body, err := io.ReadAll(r.Body)
	if err != nil {
		th.response.ValidationError(w, "无法读取请求体")
		return
	}

	// 尝试解析为批量同步请求
	if err := json.Unmarshal(body, &batchReq); err == nil && len(batchReq.Todos) > 0 {
		// 处理批量同步请求
		th.handleBatchSync(w, userUUID, &batchReq)
		return
	}

	// 如果不是批量请求，尝试解析为单个创建请求
	var req models.CreateTodoRequest
	if err := json.Unmarshal(body, &req); err != nil {
		th.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 强制要求客户端提供 UUID
	if strings.TrimSpace(req.UUID) == "" {
		th.response.ValidationError(w, "uuid 不能为空，需由客户端生成")
		return
	}

	// 验证必填字段
	if strings.TrimSpace(req.Title) == "" {
		th.response.ValidationError(w, "标题不能为空")
		return
	}

	// 设置默认分类
	if req.Category == "" {
		req.Category = "未分类"
	}

	todo, err := th.todoRepo.CreateTodo(userUUID, &req)
	if err != nil {
		th.response.ServerError(w, "创建待办事项失败")
		return
	}

	th.response.Success(w, todo, "待办事项创建成功")
}

// UpdateTodo 完全替换待办事项
func (th *TodoHandler) UpdateTodo(w http.ResponseWriter, r *http.Request, userUUID string) {
	// 从URL获取UUID
	uuid := th.extractUUIDFromPath(r.URL.Path)
	if uuid == "" {
		th.response.ValidationError(w, "无效的待办事项UUID")
		return
	}

	var req models.CreateTodoRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		th.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 验证必填字段
	if strings.TrimSpace(req.Title) == "" {
		th.response.ValidationError(w, "标题不能为空")
		return
	}

	// 转换为更新请求
	updateReq := &models.UpdateTodoRequest{
		Title:               &req.Title,
		Description:         req.Description,
		Category:            &req.Category,
		Important:           &req.Important,
		IsCompleted:         &req.IsCompleted,
		Deadline:            req.Deadline,
		RecurrenceInterval:  req.RecurrenceInterval,
		RecurrenceCount:     req.RecurrenceCount,
		RecurrenceStartDate: req.RecurrenceStartDate,
	}

	err := th.todoRepo.UpdateTodo(uuid, userUUID, updateReq)
	if err != nil {
		if strings.Contains(err.Error(), "不存在或无权限") {
			th.response.NotFoundError(w, "待办事项不存在或无权限修改")
		} else {
			th.response.ServerError(w, "更新待办事项失败")
		}
		return
	}

	th.response.Success(w, nil, "待办事项更新成功")
}

// PatchTodo 部分更新待办事项
func (th *TodoHandler) PatchTodo(w http.ResponseWriter, r *http.Request, userUUID string) {
	// 从URL获取UUID
	uuid := th.extractUUIDFromPath(r.URL.Path)
	if uuid == "" {
		th.response.ValidationError(w, "无效的待办事项UUID")
		return
	}

	var req models.UpdateTodoRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		th.response.ValidationError(w, "无效的请求格式")
		return
	}

	err := th.todoRepo.UpdateTodo(uuid, userUUID, &req)
	if err != nil {
		if strings.Contains(err.Error(), "不存在或无权限") {
			th.response.NotFoundError(w, "待办事项不存在或无权限修改")
		} else {
			th.response.ServerError(w, "更新待办事项失败")
		}
		return
	}

	th.response.Success(w, nil, "待办事项更新成功")
}

// DeleteTodo 删除待办事项
func (th *TodoHandler) DeleteTodo(w http.ResponseWriter, r *http.Request, userUUID string) {
	// 从URL获取UUID
	uuid := th.extractUUIDFromPath(r.URL.Path)
	if uuid == "" {
		th.response.ValidationError(w, "无效的待办事项UUID")
		return
	}

	// 检查是否硬删除
	hardDelete := r.URL.Query().Get("hard_delete") == "true"

	var err error
	if hardDelete {
		err = th.todoRepo.HardDeleteTodo(uuid, userUUID)
	} else {
		err = th.todoRepo.SoftDeleteTodo(uuid, userUUID)
	}

	if err != nil {
		if strings.Contains(err.Error(), "不存在或无权限") {
			th.response.NotFoundError(w, "待办事项不存在或无权限删除")
		} else {
			th.response.ServerError(w, "删除待办事项失败")
		}
		return
	}

	th.response.Success(w, nil, "待办事项删除成功")
}

// BatchOperation 批量操作
func (th *TodoHandler) BatchOperation(w http.ResponseWriter, r *http.Request, userUUID string) {
	var req models.BatchOperationRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		th.response.ValidationError(w, "无效的请求格式")
		return
	}

	if len(req.UUIDs) == 0 {
		th.response.ValidationError(w, "没有提供要操作的UUID")
		return
	}

	if len(req.UUIDs) > 100 {
		th.response.ValidationError(w, "一次最多操作100个项目")
		return
	}

	err := th.todoRepo.BatchOperation(userUUID, &req)
	if err != nil {
		th.response.ServerError(w, "批量操作失败: "+err.Error())
		return
	}

	th.response.Success(w, nil, "批量操作成功")
}

// CheckNewTodos 检查是否有新待办事项
func (th *TodoHandler) CheckNewTodos(w http.ResponseWriter, r *http.Request, userUUID string) {
	sinceTimestamp := r.URL.Query().Get("since_timestamp")
	if sinceTimestamp == "" {
		th.response.ValidationError(w, "缺少since_timestamp参数")
		return
	}

	since, err := time.Parse("2006-01-02T15:04:05", sinceTimestamp)
	if err != nil {
		th.response.ValidationError(w, "无效的时间格式")
		return
	}

	filters := map[string]interface{}{
		"created_at_after": since,
		"limit":            1,
	}

	todos, err := th.todoRepo.GetTodos(userUUID, filters)
	if err != nil {
		th.response.ServerError(w, "检查新待办事项失败")
		return
	}

	hasNew := len(todos) > 0
	th.response.Success(w, map[string]interface{}{
		"has_new": hasNew,
		"count":   len(todos),
	}, "检查完成")
}

// extractUUIDFromPath 从路径中提取UUID
func (th *TodoHandler) extractUUIDFromPath(path string) string {
	// 简单的路径解析，假设格式为 /api/todos/uuid-string
	parts := strings.Split(path, "/")
	for i, part := range parts {
		if part == "todos" && i+1 < len(parts) {
			uuid := parts[i+1]
			// 简单验证UUID格式（36个字符，包含4个连字符）
			if len(uuid) == 36 && strings.Count(uuid, "-") == 4 {
				return uuid
			}
		}
	}
	return ""
}

// handleBatchSync 处理批量同步请求
func (th *TodoHandler) handleBatchSync(w http.ResponseWriter, userUUID string, req *models.BatchSyncRequest) {
	if len(req.Todos) == 0 {
		th.response.ValidationError(w, "没有提供要同步的待办事项")
		return
	}

	if len(req.Todos) > 100 {
		th.response.ValidationError(w, "一次最多同步100个项目")
		return
	}

	var created, updated, errors, conflicts int
	var errorDetails []map[string]interface{}
	var conflictDetails []map[string]interface{}

	for i, syncItem := range req.Todos {
		// 所有同步项统一要求提供 uuid
		if strings.TrimSpace(syncItem.UUID) == "" {
			errors++
			errorDetails = append(errorDetails, map[string]interface{}{
				"index": i, "error": "uuid 不能为空", "code": "MISSING_UUID", "uuid": syncItem.UUID,
			})
			continue
		}
		// 验证用户权限
		if syncItem.UserUUID != userUUID {
			errors++
			errorDetails = append(errorDetails, map[string]interface{}{
				"index": i,
				"error": "无权限操作此项目",
				"uuid":  syncItem.UUID,
			})
			continue
		}

		// 删除优先：synced=3 表示删除（需要进行时间冲突判断）
		if syncItem.Synced != nil && *syncItem.Synced == 3 {
			if syncItem.UUID != "" {
				var serverUpdatedAt time.Time
				var serverFound bool
				serverTodo, errGet := th.todoRepo.GetTodoByUUID(syncItem.UUID, userUUID)
				if errGet == nil && serverTodo != nil {
					serverUpdatedAt = serverTodo.UpdatedAt
					serverFound = true
				}

				clientUpdatedAt := time.Time{}
				if syncItem.UpdatedAt != nil {
					clientUpdatedAt = *syncItem.UpdatedAt
				}

				// 没有客户端 updated_at 则不允许执行删除（安全策略）
				if clientUpdatedAt.IsZero() {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": "删除需要提供 updated_at", "code": "MISSING_UPDATED_AT", "uuid": syncItem.UUID,
					})
					continue
				}

				// 如果服务器存在且服务器更新时间较新，则保留服务器数据，产生冲突，不执行删除
				if serverFound && serverUpdatedAt.After(clientUpdatedAt) {
					conflicts++
					conflictDetails = append(conflictDetails, map[string]interface{}{
						"index":             i,
						"uuid":              syncItem.UUID,
						"reason":            "server_newer_keep_server",
						"server_updated_at": serverUpdatedAt,
						"client_updated_at": clientUpdatedAt,
						"action":            "skip_delete",
						"server_todo": map[string]interface{}{
							"uuid":                  serverTodo.UUID,
							"title":                 serverTodo.Title,
							"description":           serverTodo.Description,
							"category":              serverTodo.Category,
							"important":             serverTodo.Important,
							"is_completed":          serverTodo.IsCompleted,
							"deadline":              serverTodo.Deadline,
							"recurrence_interval":   serverTodo.RecurrenceInterval,
							"recurrence_count":      serverTodo.RecurrenceCount,
							"recurrence_start_date": serverTodo.RecurrenceStartDate,
							"user_uuid":             serverTodo.UserUUID,
							"created_at":            serverTodo.CreatedAt,
							"updated_at":            serverTodo.UpdatedAt,
						},
					})
					continue
				}

				// 否则执行硬删除
				err := th.todoRepo.HardDeleteTodo(syncItem.UUID, userUUID)
				if err != nil {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "DELETE_FAILED", "uuid": syncItem.UUID,
					})
				} else {
					updated++ // 删除计为更新
				}
			} else {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": "删除需要提供有效的uuid", "code": "VALIDATION_FAILED", "uuid": syncItem.UUID,
				})
			}
			continue
		}

		// 验证必填字段
		if strings.TrimSpace(syncItem.Title) == "" {
			errors++
			errorDetails = append(errorDetails, map[string]interface{}{
				"index": i,
				"error": "标题不能为空",
				"uuid":  syncItem.UUID,
			})
			continue
		}

		action := 0
		if syncItem.Synced != nil {
			action = *syncItem.Synced
		}

		// 冲突检测：如果传入 updated_at，且服务器记录较新则跳过并返回冲突
		// （简单策略：客户端 updated_at < 服务器 updated_at 判定为冲突）
		// 仅在更新或自动更新路径执行；这里需要在 repository 中提供按 ID 查询简化——已存在 GetTodoByID。

		switch action {
		case 1: // 强制创建
			createReq := th.convertToCreateRequest(&syncItem)
			_, err := th.todoRepo.CreateTodo(userUUID, createReq)
			if err != nil {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": err.Error(), "code": "CREATE_FAILED", "uuid": syncItem.UUID,
				})
			} else {
				created++
			}
		case 2: // 强制更新（需要UUID）
			if syncItem.UUID == "" {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": "更新需要提供有效uuid", "code": "VALIDATION_FAILED", "uuid": syncItem.UUID,
				})
				continue
			}

			// 冲突检测：比较更新时间，服务器新则跳过，客户端新则覆盖
			serverTodo, errGet := th.todoRepo.GetTodoByUUID(syncItem.UUID, userUUID)
			clientUpdatedAt := time.Time{}
			if syncItem.UpdatedAt != nil {
				clientUpdatedAt = *syncItem.UpdatedAt
			}
			if clientUpdatedAt.IsZero() { // 强制更新必须带 updated_at
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": "更新需要提供 updated_at", "code": "MISSING_UPDATED_AT", "uuid": syncItem.UUID,
				})
				continue
			}
			if errGet == nil && serverTodo != nil {
				serverUpdatedAt := serverTodo.UpdatedAt
				if serverUpdatedAt.After(clientUpdatedAt) { // 服务器较新，跳过更新
					conflicts++
					conflictDetails = append(conflictDetails, map[string]interface{}{
						"index":             i,
						"uuid":              syncItem.UUID,
						"reason":            "server_newer_keep_server",
						"server_updated_at": serverUpdatedAt,
						"client_updated_at": clientUpdatedAt,
						"action":            "skip_update",
						"server_todo": map[string]interface{}{
							"uuid":                  serverTodo.UUID,
							"title":                 serverTodo.Title,
							"description":           serverTodo.Description,
							"category":              serverTodo.Category,
							"important":             serverTodo.Important,
							"is_completed":          serverTodo.IsCompleted,
							"deadline":              serverTodo.Deadline,
							"recurrence_interval":   serverTodo.RecurrenceInterval,
							"recurrence_count":      serverTodo.RecurrenceCount,
							"recurrence_start_date": serverTodo.RecurrenceStartDate,
							"user_uuid":             serverTodo.UserUUID,
							"created_at":            serverTodo.CreatedAt,
							"updated_at":            serverTodo.UpdatedAt,
						},
					})
					continue
				}
			}
			updateReq := th.convertToUpdateRequest(&syncItem)
			err := th.todoRepo.UpdateTodo(syncItem.UUID, userUUID, updateReq)
			if err != nil {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": err.Error(), "code": "UPDATE_FAILED", "uuid": syncItem.UUID,
				})
			} else {
				updated++
			}
		default: // 自动模式：有UUID则更新，无UUID则创建
			if syncItem.UUID != "" {
				// 自动模式下也进行冲突检测
				serverTodo, errGet := th.todoRepo.GetTodoByUUID(syncItem.UUID, userUUID)
				clientUpdatedAt := time.Time{}
				if syncItem.UpdatedAt != nil {
					clientUpdatedAt = *syncItem.UpdatedAt
				}
				if clientUpdatedAt.IsZero() { // 自动模式更新同样要求 updated_at，避免覆盖未知新旧
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": "自动模式更新需要提供 updated_at", "code": "MISSING_UPDATED_AT", "uuid": syncItem.UUID,
					})
					continue
				}
				if errGet == nil && serverTodo != nil {
					serverUpdatedAt := serverTodo.UpdatedAt
					if serverUpdatedAt.After(clientUpdatedAt) { // 服务器较新
						conflicts++
						conflictDetails = append(conflictDetails, map[string]interface{}{
							"index":             i,
							"uuid":              syncItem.UUID,
							"reason":            "server_newer_keep_server",
							"server_updated_at": serverUpdatedAt,
							"client_updated_at": clientUpdatedAt,
							"action":            "skip_update",
							"server_todo": map[string]interface{}{
								"uuid":                  serverTodo.UUID,
								"title":                 serverTodo.Title,
								"description":           serverTodo.Description,
								"category":              serverTodo.Category,
								"important":             serverTodo.Important,
								"is_completed":          serverTodo.IsCompleted,
								"deadline":              serverTodo.Deadline,
								"recurrence_interval":   serverTodo.RecurrenceInterval,
								"recurrence_count":      serverTodo.RecurrenceCount,
								"recurrence_start_date": serverTodo.RecurrenceStartDate,
								"user_uuid":             serverTodo.UserUUID,
								"created_at":            serverTodo.CreatedAt,
								"updated_at":            serverTodo.UpdatedAt,
							},
						})
						continue
					}
				}
				updateReq := th.convertToUpdateRequest(&syncItem)
				err := th.todoRepo.UpdateTodo(syncItem.UUID, userUUID, updateReq)
				if err != nil {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "UPDATE_FAILED", "uuid": syncItem.UUID,
					})
				} else {
					updated++
				}
			} else {
				createReq := th.convertToCreateRequest(&syncItem)
				_, err := th.todoRepo.CreateTodo(userUUID, createReq)
				if err != nil {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "CREATE_FAILED", "uuid": syncItem.UUID,
					})
				} else {
					created++
				}
			}
		}
	}

	// 返回处理结果
	summary := map[string]interface{}{
		"created":          created,
		"updated":          updated,
		"conflicts":        conflicts,
		"error_count":      len(errorDetails),
		"errors":           errorDetails,
		"conflict_details": conflictDetails,
	}

	message := "批量同步完成"
	if conflicts > 0 && len(errorDetails) == 0 {
		message = "批量同步完成（包含冲突，已按较新版本保留）"
	} else if conflicts > 0 && len(errorDetails) > 0 {
		message = "批量同步完成（有冲突与错误）"
	} else if len(errorDetails) > 0 {
		message = "批量同步完成，但有部分项目处理失败"
	}

	th.response.Success(w, map[string]interface{}{
		"summary": summary,
	}, message)
}

// convertToCreateRequest 将同步项目转换为创建请求
func (th *TodoHandler) convertToCreateRequest(syncItem *models.SyncTodoItem) *models.CreateTodoRequest {
	req := &models.CreateTodoRequest{
		Title:       syncItem.Title,
		Description: syncItem.Description,
		Category:    syncItem.Category,
		Important:   syncItem.Important,
		IsCompleted: syncItem.IsCompleted,
	}
	// 携带客户端 uuid
	if syncItem.UUID != "" {
		req.UUID = syncItem.UUID
	}

	// 处理时间字段（支持毫秒或字符串，由 time.Time 解析）
	if syncItem.Deadline != nil {
		deadline := *syncItem.Deadline
		req.Deadline = &deadline
	}

	if syncItem.RecurrenceStartDate != nil && *syncItem.RecurrenceStartDate != "" {
		if startDate, err := time.Parse("2006-01-02", *syncItem.RecurrenceStartDate); err == nil {
			req.RecurrenceStartDate = &startDate
		}
	}

	// 处理重复相关字段
	if syncItem.RecurrenceInterval != nil {
		req.RecurrenceInterval = syncItem.RecurrenceInterval
	}
	if syncItem.RecurrenceCount != nil {
		req.RecurrenceCount = syncItem.RecurrenceCount
	}

	return req
}

// convertToUpdateRequest 将同步项目转换为更新请求
func (th *TodoHandler) convertToUpdateRequest(syncItem *models.SyncTodoItem) *models.UpdateTodoRequest {
	req := &models.UpdateTodoRequest{
		Title:       &syncItem.Title,
		Description: syncItem.Description,
		Category:    &syncItem.Category,
		Important:   &syncItem.Important,
		IsCompleted: &syncItem.IsCompleted,
	}

	// 处理时间字段（支持毫秒或字符串）
	if syncItem.Deadline != nil {
		if syncItem.Deadline != nil {
			deadline := *syncItem.Deadline
			req.Deadline = &deadline
		}
	}

	if syncItem.RecurrenceStartDate != nil && *syncItem.RecurrenceStartDate != "" {
		if startDate, err := time.Parse("2006-01-02", *syncItem.RecurrenceStartDate); err == nil {
			req.RecurrenceStartDate = &startDate
		}
	}

	// 处理重复相关字段
	if syncItem.RecurrenceInterval != nil {
		req.RecurrenceInterval = syncItem.RecurrenceInterval
	}
	if syncItem.RecurrenceCount != nil {
		req.RecurrenceCount = syncItem.RecurrenceCount
	}

	return req
}
