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
#include <QStandardPaths>
#include <filesystem>

BackupManager::BackupManager(QObject *parent)
    : QObject(parent),                 //
      m_config(Config::GetInstance()), //
      m_backupTimer(new QTimer(this))  //
{
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
    namespace fs = std::filesystem;
    try {
        // 获取备份路径
        std::string dir = m_config.get("backup/autoBackupPath", 获取默认备份路径()).toString().toStdString();
        // 如果备份目录不存在，且不是目录，创建它
        if (!std::filesystem::exists(dir)) {
            if (!std::filesystem::create_directories(dir)) {
                qCritical() << "无法创建目录:" << dir;
                return false;
            }
        } else if (!std::filesystem::is_directory(dir)) {
            if (!std::filesystem::create_directories(dir)) {
                qCritical() << "无法创建目录:" << dir;
                return false;
            }
        }

        // 生成备份路径
        std::string ConfigBackupFilePath = dir + "/" + 生成备份路径("config");
        std::string DatabaseBackupFilePath = dir + "/" + 生成备份路径("Qdatabase");

        // 执行配置文件备份
        std::vector<std::string> excludeKeys = {"proxy"};
        bool configBackupSuccess = m_config.exportToJsonFile(ConfigBackupFilePath, excludeKeys);
        // 执行数据库备份
        bool dbBackupSuccess = false;
        // TODO: dbBackupSuccess =Database::GetInstance().exportDatabaseToJsonFile(QString::fromStdString(DatabaseBackupFilePath));

        if (configBackupSuccess && dbBackupSuccess) {
            // 更新最后备份时间
            QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
            m_config.save("backup/lastBackupTime", currentTime);

            // 清理旧的备份文件
            int maxFiles = m_config.get("backup/maxBackupFiles", 5).toInt();
            清理旧备份文件(dir, maxFiles);

            qInfo() << "备份成功完成:";
            qInfo() << "\t配置文件备份路径:" << QString::fromStdString(ConfigBackupFilePath);
            qInfo() << "\t数据库备份路径:" << QString::fromStdString(DatabaseBackupFilePath);
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
bool BackupManager::清理旧备份文件(const std::string &backupDir, int maxFiles) {
    namespace fs = std::filesystem;

    try {
        fs::path backupPath(backupDir);

        // 检查目录是否存在
        if (!fs::exists(backupPath) || !fs::is_directory(backupPath)) {
            return true;
        }

        // 存储文件信息的结构体
        struct FileInfo {
            fs::path path;
            fs::file_time_type lastWriteTime;

            bool operator<(const FileInfo &other) const {
                return lastWriteTime > other.lastWriteTime; // 按时间降序排列（最新的在前）
            }
        };

        std::vector<FileInfo> configFiles;
        std::vector<FileInfo> dbFiles;

        // 遍历目录获取备份文件
        for (const auto &entry : fs::directory_iterator(backupPath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();

                if (filename.find("_config.json") != std::string::npos) {
                    configFiles.push_back({entry.path(), entry.last_write_time()});
                } else if (filename.find("_database.json") != std::string::npos) {
                    dbFiles.push_back({entry.path(), entry.last_write_time()});
                }
            }
        }

        // 按时间排序（最新的在前）
        std::sort(configFiles.begin(), configFiles.end());
        std::sort(dbFiles.begin(), dbFiles.end());

        // 删除超出数量限制的文件
        auto deleteOldFiles = [this](std::vector<FileInfo> &files, int maxCount) {
            while (static_cast<int>(files.size()) > maxCount) {
                const auto &oldestFile = files.back();
                std::error_code ec;
                if (fs::remove(oldestFile.path, ec)) {
                    qDebug() << "已删除旧备份文件:" << QString::fromStdString(oldestFile.path.string());
                } else {
                    qWarning() << "无法删除旧备份文件:" << QString::fromStdString(oldestFile.path.string())
                               << "错误:" << ec.message();
                }
                files.pop_back();
            }
        };

        deleteOldFiles(configFiles, maxFiles);
        deleteOldFiles(dbFiles, maxFiles);

        return true;
    } catch (const std::exception &e) {
        qCritical() << "清理备份文件时发生异常:" << e.what();
        return false;
    }
}

/**
 * @brief 生成备份路径
 * @param fileType 备份文件类型（config/Qdatabase）
 * @return 备份路径
 */
std::string BackupManager::生成备份路径(const QString &fileType) const {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_备份_%2_%3.json").arg(APP_NAME, timestamp, fileType).toStdString();
}

/**
 * @brief 获取默认备份路径
 * @return 默认备份路径
 */
QString BackupManager::获取默认备份路径() const {
    std::filesystem::path documentsPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString();
    return QString::fromStdString((documentsPath / APP_NAME / "backups").string());
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