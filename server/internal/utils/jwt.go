package utils

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/google/uuid"
	"golang.org/x/crypto/bcrypt"
)

// JWTClaims JWT声明
type JWTClaims struct {
	UserID   int    `json:"user_id"`
	UserUUID string `json:"user_uuid"`
	Username string `json:"username"`
	Email    string `json:"email"`
	JTI      string `json:"jti"` // JWT ID (防止重放攻击)
	jwt.RegisteredClaims
}

// JWTHelper JWT助手
type JWTHelper struct {
	secret []byte
	leeway time.Duration
}

// NewJWTHelper 创建新的JWT助手
func NewJWTHelper(secret string, leeway time.Duration) *JWTHelper {
	if leeway == 0 {
		leeway = 60 * time.Second // 默认60秒容错时间
	}
	return &JWTHelper{
		secret: []byte(secret),
		leeway: leeway,
	}
}

// GenerateToken 生成JWT令牌
func (j *JWTHelper) GenerateToken(userID int, userUUID, username, email string, expiry time.Duration) (string, error) {
	now := time.Now()
	jti := uuid.New().String()

	claims := JWTClaims{
		UserID:   userID,
		UserUUID: userUUID,
		Username: username,
		Email:    email,
		JTI:      jti,
		RegisteredClaims: jwt.RegisteredClaims{
			IssuedAt:  jwt.NewNumericDate(now),
			ExpiresAt: jwt.NewNumericDate(now.Add(expiry)),
			NotBefore: jwt.NewNumericDate(now),
			Issuer:    "todo-app",
			Subject:   userUUID,
		},
	}

	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	return token.SignedString(j.secret)
}

// ValidateToken 验证JWT令牌
func (j *JWTHelper) ValidateToken(tokenString string) (*JWTClaims, error) {
	token, err := jwt.ParseWithClaims(tokenString, &JWTClaims{}, func(token *jwt.Token) (interface{}, error) {
		if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("意外的签名方式: %v", token.Header["alg"])
		}
		return j.secret, nil
	})

	if err != nil {
		return nil, err
	}

	if claims, ok := token.Claims.(*JWTClaims); ok && token.Valid {
		// 检查是否在容错时间内
		if time.Until(claims.ExpiresAt.Time) < -j.leeway {
			return nil, fmt.Errorf("令牌已过期")
		}
		return claims, nil
	}

	return nil, fmt.Errorf("无效的令牌")
}

// GetTokenIssuedTime 获取token的签发时间（不验证token有效性）
func (j *JWTHelper) GetTokenIssuedTime(tokenString string) (time.Time, error) {
	token, err := jwt.ParseWithClaims(tokenString, &JWTClaims{}, func(token *jwt.Token) (interface{}, error) {
		if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("意外的签名方式: %v", token.Header["alg"])
		}
		return j.secret, nil
	}, jwt.WithoutClaimsValidation())

	if err != nil {
		return time.Time{}, err
	}

	if claims, ok := token.Claims.(*JWTClaims); ok {
		if claims.IssuedAt != nil {
			return claims.IssuedAt.Time, nil
		}
		return time.Time{}, fmt.Errorf("令牌中没有签发时间")
	}

	return time.Time{}, fmt.Errorf("无法解析令牌声明")
}

// HashPassword 哈希密码
func HashPassword(password string) (string, error) {
	bytes, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	return string(bytes), err
}

// CheckPassword 检查密码
func CheckPassword(hashedPassword, password string) bool {
	err := bcrypt.CompareHashAndPassword([]byte(hashedPassword), []byte(password))
	return err == nil
}

// GenerateUUID 生成安全的UUID
func GenerateUUID() string {
	return uuid.New().String()
}

// GenerateSecureRandomString 生成安全的随机字符串
func GenerateSecureRandomString(length int) (string, error) {
	bytes := make([]byte, length)
	if _, err := rand.Read(bytes); err != nil {
		return "", err
	}
	return hex.EncodeToString(bytes), nil
}
