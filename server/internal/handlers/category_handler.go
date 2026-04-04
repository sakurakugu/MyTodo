package handlers

import (
	"encoding/json"
	"io"
	"net/http"
	"strings"

	"MyTodo/internal/database"
	"MyTodo/internal/middleware"
	"MyTodo/internal/models"
	"MyTodo/internal/utils"
)

// CategoryHandler 分类处理器
type CategoryHandler struct {
	categoryRepo *database.CategoryRepository
	response     *utils.ResponseHelper
}

// NewCategoryHandler 创建分类处理器
func NewCategoryHandler() *CategoryHandler {
	return &CategoryHandler{
		categoryRepo: database.NewCategoryRepository(),
		response:     utils.ResponseHelperInstance,
	}
}

// HandleCategories 统一分类处理入口
func (ch *CategoryHandler) HandleCategories(w http.ResponseWriter, r *http.Request) {
	// 获取用户信息
	user, ok := middleware.GetUserFromContext(r.Context())
	if !ok {
		ch.response.UnauthorizedError(w, "未授权访问")
		return
	}

	switch r.Method {
	case http.MethodGet:
		ch.GetCategories(w, r, user.UserUUID)
	case http.MethodPost:
		// 支持批量同步：如果 body 中含有 categories 数组则走批量逻辑
		body, _ := io.ReadAll(r.Body)
		// 需要再次使用，所以复制一份
		var batchReq models.BatchCategorySyncRequest
		if err := json.Unmarshal(body, &batchReq); err == nil && len(batchReq.Categories) > 0 {
			ch.BatchSyncCategories(w, body, user.UserUUID)
			return
		}
		// 单一创建逻辑
		r.Body.Close()
		// 重新恢复 body 供下游解析（此处简单重新构造）
		r.Body = io.NopCloser(strings.NewReader(string(body)))
		ch.CreateCategory(w, r, user.UserUUID)
	case http.MethodPut:
		ch.UpdateCategory(w, r, user.UserUUID)
	case http.MethodDelete:
		ch.DeleteCategory(w, r, user.UserUUID)
	default:
		ch.response.MethodNotAllowedError(w, []string{
			http.MethodGet, http.MethodPost, http.MethodPut, http.MethodDelete,
		})
	}
}

// GetCategories 获取用户的所有分类
func (ch *CategoryHandler) GetCategories(w http.ResponseWriter, r *http.Request, userUUID string) {
	categories, err := ch.categoryRepo.GetAllCategories(userUUID)
	if err != nil {
		ch.response.ServerError(w, "获取分类列表失败")
		return
	}

	ch.response.Success(w, map[string]interface{}{
		"categories": categories,
	}, "获取分类列表成功")
}

// CreateCategory 创建新分类
func (ch *CategoryHandler) CreateCategory(w http.ResponseWriter, r *http.Request, userUUID string) {
	var req models.CreateCategoryRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ch.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 强制要求客户端提供 UUID
	if strings.TrimSpace(req.UUID) == "" {
		ch.response.ValidationError(w, "uuid 不能为空，需由客户端生成")
		return
	}

	// 验证输入
	if strings.TrimSpace(req.Name) == "" {
		ch.response.ValidationError(w, "分类名称不能为空")
		return
	}

	req.Name = strings.TrimSpace(req.Name)

	// 第二个参数传入客户端指定的 uuid
	category, err := ch.categoryRepo.CreateCategory(userUUID, req.UUID, req.Name)
	if err != nil {
		if strings.Contains(err.Error(), "已存在") {
			ch.response.ValidationError(w, err.Error())
		} else {
			ch.response.ServerError(w, "创建分类失败")
		}
		return
	}

	ch.response.Success(w, map[string]interface{}{
		"uuid": category.UUID,
	}, "分类创建成功")
}

// UpdateCategory 更新分类名称
func (ch *CategoryHandler) UpdateCategory(w http.ResponseWriter, r *http.Request, userUUID string) {
	var req models.UpdateCategoryRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ch.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 验证输入
	if strings.TrimSpace(req.Name) == "" {
		ch.response.ValidationError(w, "分类名称不能为空")
		return
	}

	if req.UUID == nil {
		ch.response.ValidationError(w, "缺少必要参数：uuid 和 name")
		return
	}

	req.Name = strings.TrimSpace(req.Name)

	err := ch.categoryRepo.UpdateCategory(userUUID, &req)
	if err != nil {
		if strings.Contains(err.Error(), "不存在或无权限") {
			ch.response.NotFoundError(w, err.Error())
		} else if strings.Contains(err.Error(), "已存在") {
			ch.response.ValidationError(w, err.Error())
		} else {
			ch.response.ServerError(w, "更新分类失败")
		}
		return
	}

	ch.response.Success(w, nil, "分类更新成功")
}

// DeleteCategory 删除分类
func (ch *CategoryHandler) DeleteCategory(w http.ResponseWriter, r *http.Request, userUUID string) {
	// 从查询参数获取UUID
	uuidStr := r.URL.Query().Get("uuid")

	if uuidStr == "" {
		ch.response.ValidationError(w, "缺少分类 UUID")
		return
	}

	// 通过UUID删除
	// 删除分类（仓储层方法 DeleteCategory）
	err := ch.categoryRepo.DeleteCategory(uuidStr, userUUID)

	if err != nil {
		if strings.Contains(err.Error(), "不存在或无权限") {
			ch.response.NotFoundError(w, err.Error())
		} else if strings.Contains(err.Error(), "不能删除") {
			ch.response.ValidationError(w, err.Error())
		} else {
			ch.response.ServerError(w, "删除分类失败: "+err.Error())
		}
		return
	}

	ch.response.Success(w, nil, "分类删除成功")
}

// BatchSyncCategories 处理批量分类同步
// 支持的操作：
// synced=1 -> 创建
// synced=2 -> 更新（按uuid 或 name 匹配，优先 uuid）
// synced=3 -> 删除（按uuid）
// 其他或缺省 -> upsert (存在则忽略/更新名称差异，不存在则创建)
func (ch *CategoryHandler) BatchSyncCategories(w http.ResponseWriter, rawBody []byte, userUUID string) {
	var batchReq models.BatchCategorySyncRequest
	if err := json.Unmarshal(rawBody, &batchReq); err != nil {
		ch.response.ValidationError(w, "无效的批量分类同步格式")
		return
	}
	if len(batchReq.Categories) == 0 {
		ch.response.ValidationError(w, "没有提供要同步的分类")
		return
	}

	var created, updated, deleted, errors int
	var errorDetails []map[string]interface{}

	for i, c := range batchReq.Categories {
		rawUUID := strings.TrimSpace(c.UUID)
		rawName := strings.TrimSpace(c.Name)
		if rawUUID == "" || rawName == "" {
			errors++
			errorDetails = append(errorDetails, map[string]interface{}{
				"index": i,
				"error": "uuid 或 name 不能为空",
				"code":  "VALIDATION_FAILED",
				"uuid":  c.UUID,
			})
			continue
		}

		// 内部压缩空白，与仓储层保持一致（避免同一个名称不同空格形式导致多次创建）
		normalizedName := strings.Join(strings.Fields(rawName), " ")

		action := 0
		if c.Synced != nil {
			action = *c.Synced
		}

		switch action {
		case 1: // 创建
			// 使用客户端提供的 UUID 进行创建，保证后续更新 (synced=2) 能正确匹配
			_, err := ch.categoryRepo.CreateCategory(userUUID, rawUUID, normalizedName)
			if err != nil {
				if strings.Contains(err.Error(), "已存在") {
					// 忽略
				} else {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "CREATE_FAILED", "uuid": c.UUID,
					})
				}
			} else {
				created++
			}
		case 2: // 更新
			updReq := models.UpdateCategoryRequest{Name: normalizedName}
			if c.UUID != "" {
				uuidCopy := c.UUID
				updReq.UUID = &uuidCopy
			}
			if err := ch.categoryRepo.UpdateCategory(userUUID, &updReq); err != nil {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": err.Error(), "code": "UPDATE_FAILED", "uuid": c.UUID,
				})
			} else {
				updated++
			}
		case 3: // 删除
			if c.UUID != "" {
				if err := ch.categoryRepo.DeleteCategory(c.UUID, userUUID); err != nil {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "DELETE_FAILED", "uuid": c.UUID,
					})
				} else {
					deleted++
				}
			} else {
				errors++
				errorDetails = append(errorDetails, map[string]interface{}{
					"index": i, "error": "删除操作需要提供uuid", "code": "VALIDATION_FAILED", "uuid": c.UUID,
				})
			}
		default: // upsert
			// 使用客户端提供的 UUID 进行创建，保证后续更新 (synced=2) 能正确匹配
			_, err := ch.categoryRepo.CreateCategory(userUUID, rawUUID, normalizedName)
			if err != nil {
				if strings.Contains(err.Error(), "已存在") {
					// 忽略
				} else {
					errors++
					errorDetails = append(errorDetails, map[string]interface{}{
						"index": i, "error": err.Error(), "code": "CREATE_FAILED", "uuid": c.UUID,
					})
				}
			} else {
				created++
			}
		}
	}

	summary := map[string]interface{}{
		"created": created,
		"updated": updated,
		"deleted": deleted,
		"errors":  errorDetails,
	}

	msg := "批量分类同步完成"
	if errors > 0 {
		msg = "批量分类同步完成，但有部分分类处理失败"
	}

	ch.response.Success(w, map[string]interface{}{"summary": summary}, msg)
}
