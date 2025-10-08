/**
 * @file backup_manager.cpp
 * @brief BackupManager类的实现文件
 *
 * 该文件实现了BackupManager类，专门用于管理应用程序的备份功能。
 *
 * @author Sakurakugu
 * @date 2025-10-08 23:01:00(UTC+8) 周三
 */

#include "backup_manager.h"
#include "config.h"
#include "database.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

BackupManager::BackupManager(QObject *parent)
    : QObject(parent), m_config(Config::GetInstance()), m_backupTimer(new QTimer(this)) {
    // 初始化自动备份定时器
    m_backupTimer->setSingleShot(false);
    m_backupTimer->setInterval(60 * 60 * 1000); // 每小时检查一次
    connect(m_backupTimer, &QTimer::timeout, this, &BackupManager::checkAndPerformAutoBackup);

    初始化();
}

BackupManager::~BackupManager() {}

/**
 * @brief 初始化备份管理器
 * 设置定时器并根据配置启动自动备份
 */
void BackupManager::初始化() {
    if (m_config.get("backup/autoBackupPath").toString().isEmpty()) {
        m_config.save("backup/autoBackupPath", 获取默认备份路径());
    }
    // 如果启用了自动备份，启动定时器
    if (m_config.get("backup/autoBackupEnabled", false).toBool()) {
        启动自动备份定时器();
    }
}

/**
 * @brief 执行备份操作
 * @return 备份是否成功
 */
bool BackupManager::执行备份() {
    try {
        // 获取备份路径
        QString backupPath = m_config.get("backup/autoBackupPath", 获取默认备份路径()).toString();

        // 确保备份目录存在
        QDir backupDir(backupPath);
        if (!backupDir.exists()) {
            if (!backupDir.mkpath(".")) {
                qWarning() << "无法创建备份目录:" << backupPath;
                return false;
            }
        }

        // 生成备份路径
        QString ConfigBackupFilePath = backupDir.absoluteFilePath(生成备份路径("config"));
        QString DatabaseBackupFilePath = backupDir.absoluteFilePath(生成备份路径("database"));

        // 执行配置文件备份
        std::vector<std::string> excludeKeys = {"proxy"};
        bool configBackupSuccess = m_config.exportToJsonFile(ConfigBackupFilePath.toStdString(), excludeKeys);
        // 执行数据库备份
        bool dbBackupSuccess = Database::GetInstance().exportDatabaseToJsonFile(DatabaseBackupFilePath);

        if (configBackupSuccess && dbBackupSuccess) {
            // 更新最后备份时间
            QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
            m_config.save("backup/lastBackupTime", currentTime);

            // 清理旧的备份文件
            int maxFiles = m_config.get("backup/maxBackupFiles", 5).toInt();
            清理旧备份文件(backupPath, maxFiles);

            qInfo() << "备份成功完成:";
            qInfo() << "\t配置文件备份路径:" << ConfigBackupFilePath;
            qInfo() << "\t数据库备份路径:" << DatabaseBackupFilePath;
            emit backupCompleted(true, "备份成功完成");
            return true;
        } else {
            qWarning() << "备份失败";
            emit backupCompleted(false, "备份失败");
            return false;
        }
    } catch (const std::exception &e) {
        qCritical() << "备份过程中发生异常:" << e.what();
        emit backupCompleted(false, QString("备份过程中发生异常: %1").arg(e.what()));
        return false;
    }
}

/**
 * @brief 清理旧的备份文件
 * @param backupDir 备份目录路径
 * @param maxFiles 最大保留文件数
 * @return 清理是否成功
 */
bool BackupManager::清理旧备份文件(const QString &backupDir, int maxFiles) {
    QDir dir(backupDir);
    if (!dir.exists()) {
        return true;
    }

    // 获取所有备份文件
    QStringList filters;
    filters << "*_config.json" << "*_database.json";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    // 按类型分组文件
    QFileInfoList configFiles;
    QFileInfoList dbFiles;

    for (const QFileInfo &fileInfo : fileList) {
        if (fileInfo.fileName().contains("_config.json")) {
            configFiles.append(fileInfo);
        } else if (fileInfo.fileName().contains("_database.json")) {
            dbFiles.append(fileInfo);
        }
    }

    // 删除超出数量限制的文件
    auto deleteOldFiles = [this](QFileInfoList &files, int maxCount) {
        while (files.size() > maxCount) {
            QFileInfo oldestFile = files.takeLast();
            if (!QFile::remove(oldestFile.absoluteFilePath())) {
                qWarning() << "无法删除旧备份文件:" << oldestFile.absoluteFilePath();
            } else {
                qDebug() << "已删除旧备份文件:" << oldestFile.absoluteFilePath();
            }
        }
    };

    deleteOldFiles(configFiles, maxFiles);
    deleteOldFiles(dbFiles, maxFiles);

    return true;
}

/**
 * @brief 生成备份路径
 * @param fileType 备份文件类型（config/database）
 * @return 备份路径
 */
QString BackupManager::生成备份路径(const QString &fileType) const {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_备份_%2_%3.json").arg(APP_NAME, timestamp, fileType);
}

/**
 * @brief 获取默认备份路径
 * @return 默认备份路径
 */
QString BackupManager::获取默认备份路径() const {
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir(documentsPath).absoluteFilePath(QString(APP_NAME) + "/backups");
}

/**
 * @brief 设置自动备份启用状态
 * @param enabled 是否启用自动备份
 */
void BackupManager::设置自动备份启用状态(bool enabled) {
    m_config.save("backup/autoBackupEnabled", enabled);
    if (enabled) {
        启动自动备份定时器();
    } else {
        停止自动备份定时器();
    }
}

/**
 * @brief 启动自动备份定时器
 */
void BackupManager::启动自动备份定时器() {
    if (!m_backupTimer->isActive()) {
        m_backupTimer->start();
        qDebug() << "自动备份定时器已启动";
    }
}

/**
 * @brief 检查是否需要执行备份
 * @return 是否需要备份
 */
void BackupManager::停止自动备份定时器() {
    if (m_backupTimer->isActive()) {
        m_backupTimer->stop();
        qDebug() << "自动备份定时器已停止";
    }
}

bool BackupManager::检查是否需要备份() const {
    // 检查是否启用了自动备份
    if (!m_config.get("backup/autoBackupEnabled", false).toBool()) {
        return false;
    }

    // 获取备份间隔（天数）
    int intervalDays = m_config.get("backup/autoBackupInterval", 7).toInt();

    // 获取最后备份时间
    QString lastBackupTimeStr = m_config.get("backup/lastBackupTime", "").toString();
    if (lastBackupTimeStr.isEmpty()) {
        // 如果从未备份过，需要备份
        return true;
    }

    // 解析最后备份时间
    QDateTime lastBackupTime = QDateTime::fromString(lastBackupTimeStr, Qt::ISODate);
    if (!lastBackupTime.isValid()) {
        // 如果时间格式无效，需要备份
        return true;
    }

    // 计算距离上次备份的天数
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 daysSinceLastBackup = lastBackupTime.daysTo(currentTime);

    // 如果距离上次备份的天数超过了间隔，需要备份
    return daysSinceLastBackup >= intervalDays;
}

/**
 * @brief 定时器触发的备份检查槽函数
 */
void BackupManager::checkAndPerformAutoBackup() {
    if (检查是否需要备份()) {
        qInfo() << "开始执行自动备份...";
        if (执行备份()) {
            qInfo() << "自动备份完成";
        } else {
            qWarning() << "自动备份失败";
        }
    }
}