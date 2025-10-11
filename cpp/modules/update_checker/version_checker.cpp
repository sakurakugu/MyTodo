/**
 * @file version_checker.cpp
 * @brief 版本更新检查器的实现文件
 *
 * 该文件实现了VersionChecker类，提供GitHub版本检查和比较功能。
 *
 * @author Sakurakugu
 * @date 2025-10-07 19:35:00(UTC+8) 周二
 * @change 2025-10-07 19:35:00(UTC+8) 周二
 */

#include "version_checker.h"
#include "network_request.h"
#include "version.h"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QUrl>
#include <QVersionNumber>

// GitHub配置常量
const QString VersionChecker::GITHUB_API_URL = "https://api.github.com";
const QString VersionChecker::GITHUB_REPO_OWNER = "sakurakugu";
const QString VersionChecker::GITHUB_REPO_NAME = "MyTodo";

VersionChecker::VersionChecker(QObject *parent)
    : QObject(parent),                                    //
      m_currentVersion(APP_VERSION_STRING),               //
      m_hasUpdate(false),                                 //
      m_isChecking(false),                                //
      m_autoCheckEnabled(false),                          // 默认不启用自动检查版本更新
      m_checkIntervalHours(DEFAULT_CHECK_INTERVAL_HOURS), //
      m_autoCheckTimer(new QTimer(this))                  //
{

    // 连接定时器信号
    connect(m_autoCheckTimer, &QTimer::timeout, this, &VersionChecker::onAutoCheckTimer);

    // 连接网络请求信号
    connect(&NetworkRequest::GetInstance(), &NetworkRequest::requestCompleted, this,
            [this](Network::RequestType type, const QJsonObject &response) {
                if (type == Network::RequestType::UpdateCheck) {
                    解析GitHub响应(response);
                }
            });

    connect(&NetworkRequest::GetInstance(), &NetworkRequest::requestFailed, this,
            [this](Network::RequestType type, [[maybe_unused]] Network::Error error, const QString &message) {
                if (type == Network::RequestType::UpdateCheck) {
                    设置是否正在检查(false);
                    emit updateCheckFailed(message);
                    qWarning() << "版本检查失败:" << message;
                }
            });

    // 启动自动检查
    if (m_autoCheckEnabled) {
        启动自动检查定时器();
    }
}

VersionChecker::~VersionChecker() {
    停止自动检查定时器();
}

void VersionChecker::设置启用自动检查(bool enabled) {
    if (m_autoCheckEnabled != enabled) {
        m_autoCheckEnabled = enabled;
        emit isCheckingChanged();

        if (enabled)
            启动自动检查定时器();
        else
            停止自动检查定时器();
    }
}

void VersionChecker::检查最新版本() {
    if (m_isChecking)
        return;

    qInfo() << "开始检查版本更新";
    执行版本检查();
}

void VersionChecker::打开下载页面() {
    if (!m_updateUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_updateUrl));
        qInfo() << "打开下载页面:" << m_updateUrl;
    } else {
        // 如果没有具体的更新URL，打开项目发布页面
        QString projectUrl =
            QString("https://github.com/%1/%2/releases/latest").arg(GITHUB_REPO_OWNER).arg(GITHUB_REPO_NAME);
        QDesktopServices::openUrl(QUrl(projectUrl));
        qInfo() << "打开项目发布页面:" << projectUrl;
    }
}

void VersionChecker::设置检查间隔(int hours) {
    if (hours < 8760 && hours > 0 && m_checkIntervalHours != hours) { // 一年内
        m_checkIntervalHours = hours;

        // 重启定时器以应用新的间隔
        if (m_autoCheckEnabled) {
            停止自动检查定时器();
            启动自动检查定时器();
        }

        qInfo() << "设置版本检查间隔为" << hours << "小时";
    }
}

void VersionChecker::onUpdateCheckFinished() {
    设置是否正在检查(false);
    emit updateCheckCompleted(m_hasUpdate, m_latestVersion);
    qInfo() << "版本检查完成，有更新:" << m_hasUpdate << "最新版本:" << m_latestVersion;
}

void VersionChecker::onAutoCheckTimer() {
    qDebug() << "自动检查版本更新";
    检查最新版本();
}

void VersionChecker::执行版本检查() {
    设置是否正在检查(true);

    // 构建GitHub API URL
    QString apiUrl =
        QString("%1/repos/%2/%3/releases/latest").arg(GITHUB_API_URL).arg(GITHUB_REPO_OWNER).arg(GITHUB_REPO_NAME);

    // 配置网络请求
    NetworkRequest::RequestConfig config;
    config.url = apiUrl;
    config.method = "GET";
    config.requiresAuth = false; // GitHub公共API不需要认证
    config.timeout = 15000;      // 15秒超时
    config.maxRetries = 2;       // 最多重试2次

    // 设置请求头
    config.headers["Accept"] = "application/vnd.github.v3+json";
    config.headers["User-Agent"] = QString("%1/%2").arg(APP_NAME).arg(APP_VERSION_STRING);

    // 创建GitHub API自定义响应处理器
    auto githubResponseHandler = [this](const QByteArray &rawResponse, int httpStatusCode) -> QJsonObject {
        QJsonObject result;

        try {
            if (httpStatusCode == 200) {
                // 解析GitHub API响应
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(rawResponse, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "GitHub API响应JSON解析错误:" << parseError.errorString();
                    return result; // 返回空对象表示失败
                }

                QJsonObject githubResponse = doc.object();

                // 验证必要字段
                if (!githubResponse.contains("tag_name")) {
                    qWarning() << "GitHub API响应缺少tag_name字段";
                    return result;
                }

                // 直接返回GitHub API的响应数据，让解析GitHub响应方法处理
                return githubResponse;

            } else {
                qWarning() << "GitHub API请求失败，HTTP状态码:" << httpStatusCode;
                qWarning() << "响应内容:" << QString::fromUtf8(rawResponse);
            }
        } catch (const std::exception &e) {
            qWarning() << "处理GitHub API响应时发生异常:" << e.what();
        } catch (...) {
            qWarning() << "处理GitHub API响应时发生未知异常";
        }

        return result; // 返回空对象表示失败
    };

    // 使用自定义处理器发送请求
    NetworkRequest::GetInstance().sendRequest(Network::RequestType::UpdateCheck, config, githubResponseHandler);

    qDebug() << "发送版本检查请求到:" << apiUrl;
}

void VersionChecker::解析GitHub响应(const QJsonObject &response) {
    try {
        // 检查响应是否为空（自定义处理器失败时返回空对象）
        if (response.isEmpty()) {
            emit updateCheckFailed("GitHub API响应处理失败");
            设置是否正在检查(false);
            return;
        }

        // 检查响应是否包含必要字段
        if (!response.contains("tag_name")) {
            emit updateCheckFailed("GitHub API响应格式错误：缺少tag_name字段");
            设置是否正在检查(false);
            return;
        }

        // 获取版本信息
        QString tagName = response["tag_name"].toString();
        QString latestVersion = tagName;

        // 移除版本号前缀（如 "v1.0.0" -> "1.0.0"）
        if (latestVersion.startsWith("v", Qt::CaseInsensitive)) {
            latestVersion = latestVersion.mid(1);
        }

        // 获取下载URL
        QString downloadUrl = response["html_url"].toString();

        // 获取发布说明
        QString releaseNotes = response["body"].toString();
        if (releaseNotes.length() > 500) {
            releaseNotes = releaseNotes.left(500) + "...";
        }

        // 检查是否为预发布版本
        bool isPrerelease = response["prerelease"].toBool();
        if (isPrerelease) {
            qDebug() << "跳过预发布版本:" << latestVersion;
            设置是否正在检查(false);
            return;
        }

        // 更新属性
        设置最新版本(latestVersion);
        设置更新URL(downloadUrl);
        设置发布说明(releaseNotes);

        // 比较版本
        bool hasNewVersion = 比较版本号(m_currentVersion, latestVersion);
        设置是否有更新(hasNewVersion);

        onUpdateCheckFinished();

    } catch (const std::exception &e) {
        emit updateCheckFailed(QString("解析GitHub响应时发生错误: %1").arg(e.what()));
        设置是否正在检查(false);
    }
}

bool VersionChecker::比较版本号(const QString &current, const QString &latest) {
    QVersionNumber currentVer = QVersionNumber::fromString(current);
    QVersionNumber latestVer = QVersionNumber::fromString(latest);

    if (currentVer.isNull() || latestVer.isNull()) {
        qWarning() << "版本号解析失败 - 当前:" << current << "最新:" << latest;
        return false;
    }

    bool hasUpdate = latestVer > currentVer;
    qDebug() << "版本比较 - 当前:" << currentVer.toString() << "最新:" << latestVer.toString()
             << "有更新:" << hasUpdate;

    return hasUpdate;
}

void VersionChecker::设置最新版本(const QString &version) {
    if (m_latestVersion != version) {
        m_latestVersion = version;
        emit latestVersionChanged();
    }
}

void VersionChecker::设置是否有更新(bool hasUpdate) {
    if (m_hasUpdate != hasUpdate) {
        m_hasUpdate = hasUpdate;
        emit hasUpdateChanged();
    }
}

void VersionChecker::设置是否正在检查(bool checking) {
    if (m_isChecking != checking) {
        m_isChecking = checking;
        emit isCheckingChanged();
    }
}

void VersionChecker::设置更新URL(const QString &url) {
    if (m_updateUrl != url) {
        m_updateUrl = url;
        emit updateUrlChanged();
    }
}

void VersionChecker::设置发布说明(const QString &notes) {
    if (m_releaseNotes != notes) {
        m_releaseNotes = notes;
        emit releaseNotesChanged();
    }
}

void VersionChecker::启动自动检查定时器() {
    if (m_autoCheckTimer && !m_autoCheckTimer->isActive()) {
        int intervalMs = m_checkIntervalHours * 60 * 60 * 1000; // 转换为毫秒
        m_autoCheckTimer->start(intervalMs);
        qDebug() << "启动自动版本检查定时器，间隔:" << m_checkIntervalHours << "小时";

        // 立即执行一次检查
        QTimer::singleShot(1000, this, &VersionChecker::执行版本检查);
    }
}

void VersionChecker::停止自动检查定时器() {
    if (m_autoCheckTimer && m_autoCheckTimer->isActive()) {
        m_autoCheckTimer->stop();
        qDebug() << "停止自动版本检查定时器";
    }
}