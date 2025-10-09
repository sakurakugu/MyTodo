/**
 * @file backup_manager.h
 * @brief BackupManager类的头文件
 *
 * 该文件定义了BackupManager类，专门用于管理应用程序的备份功能。
 * 从Setting类中分离出来，提供更好的代码组织和维护性。
 *
 * @author Sakurakugu
 * @date 2025-10-08 23:01:00(UTC+8) 周三
 */

#pragma once

#include "logger.h"

#include <QObject>
#include <QString>
#include <QTimer>

class Config;

/**
 * @brief 备份管理器类
 *
 * 负责管理应用程序的自动备份功能，包括：
 * - 执行配置文件和数据库的备份
 * - 管理备份文件的清理
 * - 控制自动备份定时器
 * - 检查备份条件
 */
class BackupManager : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static BackupManager &GetInstance() {
        static BackupManager instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    BackupManager(const BackupManager &) = delete;
    BackupManager &operator=(const BackupManager &) = delete;
    BackupManager(BackupManager &&) = delete;
    BackupManager &operator=(BackupManager &&) = delete;

    bool 执行备份();
    bool 清理旧备份文件(const std::string &backupDir, int maxFiles);
    std::string 生成备份路径(const QString &fileType) const; // 生成备份路径
    QString 获取默认备份路径() const;
    void 设置自动备份启用状态(bool enabled);
    void 启动自动备份定时器();
    void 停止自动备份定时器();
    bool 检查是否需要备份() const;
    void 初始化();

  signals:
    void backupCompleted(bool success, const QString &message); // 备份完成信号

  private slots:
    void checkAndPerformAutoBackup();

  private:
    explicit BackupManager(QObject *parent = nullptr);
    ~BackupManager();

    Config &m_config;      ///< 配置文件对象
    QTimer *m_backupTimer; ///< 自动备份定时器
};