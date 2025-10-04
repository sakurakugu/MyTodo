package handlers

import (
	"encoding/json"
	"net/http"
	"time"

	"MyTodo/internal/config"
	"MyTodo/internal/database"
	"MyTodo/internal/models"
	"MyTodo/internal/utils"
)

// AuthHandler 认证处理器
type AuthHandler struct {
	userRepo  *database.UserRepository
	jwtHelper *utils.JWTHelper
	response  *utils.ResponseHelper
}

// NewAuthHandler 创建认证处理器
func NewAuthHandler() *AuthHandler {
	jwtHelper := utils.NewJWTHelper(
		config.GlobalConfig.Security.JWTSecret,
		60*time.Second,
	)

	return &AuthHandler{
		userRepo:  database.NewUserRepository(),
		jwtHelper: jwtHelper,
		response:  utils.ResponseHelperInstance,
	}
}

// Register 用户注册
func (ah *AuthHandler) Register(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		ah.response.MethodNotAllowedError(w, []string{http.MethodPost})
		return
	}

	var req models.RegisterRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ah.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 验证输入
	if req.Username == "" || req.Email == "" || req.Password == "" {
		ah.response.ValidationError(w, "用户名、邮箱和密码不能为空")
		return
	}

	if len(req.Password) < 6 {
		ah.response.ValidationError(w, "密码长度至少6位")
		return
	}

	// 检查用户是否已存在
	exists, err := ah.userRepo.CheckUserExists(req.Username, req.Email)
	if err != nil {
		ah.response.ServerError(w, "检查用户存在性失败")
		return
	}
	if exists {
		ah.response.ValidationError(w, "用户名或邮箱已存在")
		return
	}

	// 创建用户
	user, err := ah.userRepo.CreateUser(req.Username, req.Email, req.Password)
	if err != nil {
		ah.response.ServerError(w, "创建用户失败")
		return
	}

	// 生成令牌
	accessToken, err := ah.jwtHelper.GenerateToken(
		user.ID, user.UUID, user.Username, user.Email,
		time.Duration(config.GlobalConfig.Security.SessionTimeout)*time.Second,
	)
	if err != nil {
		ah.response.ServerError(w, "生成访问令牌失败")
		return
	}

	refreshToken, err := ah.jwtHelper.GenerateToken(
		user.ID, user.UUID, user.Username, user.Email,
		time.Duration(config.GlobalConfig.Security.RefreshTimeout)*time.Second,
	)
	if err != nil {
		ah.response.ServerError(w, "生成刷新令牌失败")
		return
	}

	tokenResponse := models.TokenResponse{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		ExpiresIn:    config.GlobalConfig.Security.SessionTimeout,
		TokenType:    "Bearer",
		User:         &models.UserInfo{UUID: user.UUID, Username: user.Username, Email: user.Email},
	}

	ah.response.Success(w, tokenResponse, "用户注册成功")
}

// Login 用户登录
func (ah *AuthHandler) Login(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		ah.response.MethodNotAllowedError(w, []string{http.MethodPost})
		return
	}

	var req models.LoginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ah.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 验证输入
	if req.Username == "" || req.Password == "" {
		ah.response.ValidationError(w, "用户名和密码不能为空")
		return
	}

	// 验证用户
	user, err := ah.userRepo.ValidateUser(req.Username, req.Password)
	if err != nil {
		ah.response.UnauthorizedError(w, "用户名或密码错误", "LOGIN_FAILED")
		return
	}

	// 生成令牌
	accessToken, err := ah.jwtHelper.GenerateToken(
		user.ID, user.UUID, user.Username, user.Email,
		time.Duration(config.GlobalConfig.Security.SessionTimeout)*time.Second,
	)
	if err != nil {
		ah.response.ServerError(w, "生成访问令牌失败")
		return
	}

	refreshToken, err := ah.jwtHelper.GenerateToken(
		user.ID, user.UUID, user.Username, user.Email,
		time.Duration(config.GlobalConfig.Security.RefreshTimeout)*time.Second,
	)
	if err != nil {
		ah.response.ServerError(w, "生成刷新令牌失败")
		return
	}

	tokenResponse := models.TokenResponse{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		ExpiresIn:    config.GlobalConfig.Security.SessionTimeout,
		TokenType:    "Bearer",
		User:         &models.UserInfo{UUID: user.UUID, Username: user.Username, Email: user.Email},
	}

	ah.response.Success(w, tokenResponse, "登录成功")
}

// RefreshToken 刷新令牌
func (ah *AuthHandler) RefreshToken(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		ah.response.MethodNotAllowedError(w, []string{http.MethodPost})
		return
	}

	var req models.RefreshTokenRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ah.response.ValidationError(w, "无效的请求格式")
		return
	}

	// 验证刷新令牌
	claims, err := ah.jwtHelper.ValidateToken(req.RefreshToken)
	if err != nil {
		ah.response.UnauthorizedError(w, "无效的刷新令牌")
		return
	}

	// 获取用户信息
	user, err := ah.userRepo.GetUserByUUID(claims.UserUUID)
	if err != nil {
		ah.response.UnauthorizedError(w, "用户不存在")
		return
	}

	// 生成新的访问令牌
	accessToken, err := ah.jwtHelper.GenerateToken(
		user.ID, user.UUID, user.Username, user.Email,
		time.Duration(config.GlobalConfig.Security.SessionTimeout)*time.Second,
	)
	if err != nil {
		ah.response.ServerError(w, "生成访问令牌失败")
		return
	}

	tokenResponse := models.TokenResponse{
		AccessToken:  accessToken,
		RefreshToken: req.RefreshToken, // 保持原有的刷新令牌
		ExpiresIn:    config.GlobalConfig.Security.SessionTimeout,
		TokenType:    "Bearer",
	}

	ah.response.Success(w, tokenResponse, "令牌刷新成功")
}

// HandleAuth 统一认证处理入口
func (ah *AuthHandler) HandleAuth(w http.ResponseWriter, r *http.Request) {
	action := r.URL.Query().Get("action")

	switch action {
	case "register":
		ah.Register(w, r)
	case "login":
		ah.Login(w, r)
	case "refresh":
		ah.RefreshToken(w, r)
	default:
		ah.response.ValidationError(w, "无效的操作类型")
	}
}
