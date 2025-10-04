package config

import (
	"database/sql"
	"fmt"
	"log"
	"os"
	"strconv"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/joho/godotenv"
)

type Config struct {
	Database DatabaseConfig
	Security SecurityConfig
	App      AppConfig
}

type DatabaseConfig struct {
	Host       string
	DBName     string
	UserDBName string
	Username   string
	Password   string
	Charset    string
	DB         *sql.DB
	UserDB     *sql.DB
}

type SecurityConfig struct {
	JWTSecret      string
	EncryptionKey  string
	APIRateLimit   int
	SessionTimeout int
	RefreshTimeout int
}

type AppConfig struct {
	Debug    bool
	LogLevel string
	Timezone string
	Port     string
}

var GlobalConfig *Config

// LoadConfig 加载配置
func LoadConfig() (*Config, error) {
	// 尝试加载 .env 文件
	if err := godotenv.Load(); err != nil {
		log.Println("未找到 .env 文件，使用默认配置")
	}

	config := &Config{
		Database: DatabaseConfig{
			Host:       getEnvOrDefault("DB_HOST", "localhost"),
			DBName:     getEnvOrDefault("DB_NAME", "todo_app"),
			UserDBName: getEnvOrDefault("DB_USER_DB_NAME", "user_db"),
			Username:   getEnvOrDefault("DB_USERNAME", "robot_todo_user"),
			Password:   getEnvOrDefault("DB_PASSWORD", "fpVCJQ37WabckejkCBFR"),
			Charset:    getEnvOrDefault("DB_CHARSET", "utf8mb4"),
		},
		Security: SecurityConfig{
			JWTSecret:      getEnvOrDefault("JWT_SECRET", "Your-Strong-Secret-Key-Here-Change-This-In-Production"),
			EncryptionKey:  getEnvOrDefault("ENCRYPTION_KEY", "your-encryption-key-here"),
			APIRateLimit:   getEnvIntOrDefault("API_RATE_LIMIT", 1000),
			SessionTimeout: getEnvIntOrDefault("SESSION_TIMEOUT", 3600),
			RefreshTimeout: getEnvIntOrDefault("REFRESH_TIMEOUT", 2592000),
		},
		App: AppConfig{
			Debug:    getEnvBoolOrDefault("APP_DEBUG", false),
			LogLevel: getEnvOrDefault("LOG_LEVEL", "error"),
			Timezone: getEnvOrDefault("TIMEZONE", "Asia/Shanghai"),
			Port:     getEnvOrDefault("PORT", "8080"),
		},
	}

	// 初始化数据库连接
	if err := config.initDatabase(); err != nil {
		return nil, fmt.Errorf("初始化数据库失败: %v", err)
	}

	GlobalConfig = config
	return config, nil
}

// initDatabase 初始化数据库连接
func (c *Config) initDatabase() error {
	// 初始化主数据库连接
	dsn := fmt.Sprintf("%s:%s@tcp(%s)/%s?charset=%s&parseTime=True&loc=Local",
		c.Database.Username,
		c.Database.Password,
		c.Database.Host,
		c.Database.DBName,
		c.Database.Charset,
	)

	db, err := sql.Open("mysql", dsn)
	if err != nil {
		return fmt.Errorf("打开主数据库连接失败: %v", err)
	}

	// 设置连接池参数
	db.SetMaxOpenConns(25)
	db.SetMaxIdleConns(25)
	db.SetConnMaxLifetime(5 * time.Minute)

	// 测试连接
	if err := db.Ping(); err != nil {
		return fmt.Errorf("主数据库连接测试失败: %v", err)
	}

	c.Database.DB = db
	log.Println("主数据库连接成功")

	// 初始化用户数据库连接
	userDSN := fmt.Sprintf("%s:%s@tcp(%s)/%s?charset=%s&parseTime=True&loc=Local",
		c.Database.Username,
		c.Database.Password,
		c.Database.Host,
		c.Database.UserDBName,
		c.Database.Charset,
	)

	userDB, err := sql.Open("mysql", userDSN)
	if err != nil {
		return fmt.Errorf("打开用户数据库连接失败: %v", err)
	}

	// 设置连接池参数
	userDB.SetMaxOpenConns(25)
	userDB.SetMaxIdleConns(25)
	userDB.SetConnMaxLifetime(5 * time.Minute)

	// 测试连接
	if err := userDB.Ping(); err != nil {
		return fmt.Errorf("用户数据库连接测试失败: %v", err)
	}

	c.Database.UserDB = userDB
	log.Println("用户数据库连接成功")

	return nil
}

// CloseDatabase 关闭数据库连接
func (c *Config) CloseDatabase() {
	if c.Database.DB != nil {
		c.Database.DB.Close()
	}
	if c.Database.UserDB != nil {
		c.Database.UserDB.Close()
	}
}

// 辅助函数
func getEnvOrDefault(key, defaultValue string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return defaultValue
}

func getEnvIntOrDefault(key string, defaultValue int) int {
	if value := os.Getenv(key); value != "" {
		if intValue, err := strconv.Atoi(value); err == nil {
			return intValue
		}
	}
	return defaultValue
}

func getEnvBoolOrDefault(key string, defaultValue bool) bool {
	if value := os.Getenv(key); value != "" {
		if boolValue, err := strconv.ParseBool(value); err == nil {
			return boolValue
		}
	}
	return defaultValue
}
