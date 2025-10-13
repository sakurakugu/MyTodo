/**
 * @file base_sync_server.cpp
 * @brief BaseSyncServer基类的实现文件
 *
 * 该文件实现了BaseSyncServer基类的通用同步功能。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:45:52(UTC+8) 周三
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#include "base_sync_server.h"
#include "config.h"
#include "user_auth.h"

#include <QDateTime>

BaseSyncServer::BaseSyncServer(UserAuth &userAuth, QObject *parent)
    : QObject(parent),                                 // 父对象
      m_networkRequest(NetworkRequest::GetInstance()), // 网络请求对象
      m_config(Config::GetInstance()),                 // 设置对象
      m_userAuth(userAuth),                            // 用户认证对象
      m_autoSyncTimer(new QTimer(this)),               // 自动同步定时器
      m_isSyncing(false),                              // 是否正在同步
      m_autoSyncInterval(30),                          // 自动同步间隔
      m_currentSyncDirection(Bidirectional)            // 当前同步方向
{
    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &BaseSyncServer::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &BaseSyncServer::onNetworkRequestFailed);

    // 连接自动同步定时器
    connect(m_autoSyncTimer, &QTimer::timeout, this, &BaseSyncServer::onAutoSyncTimer);

    // 连接自动同步设置变化信号
    connect(&m_userAuth, &UserAuth::firstAuthCompleted, this, [this]() { 与服务器同步(); });

    // 从设置中加载自动同步配置
    m_autoSyncInterval = m_config.get("sync/autoSyncInterval", 30).toInt();
    m_lastSyncTime = QString("1970-01-01 00:00:00");

    开启自动同步计时器();
}

BaseSyncServer::~BaseSyncServer() {}

bool BaseSyncServer::isSyncing() const {
    return m_isSyncing;
}

void BaseSyncServer::setIsSyncing(bool syncing) {
    if (m_isSyncing != syncing) {
        m_isSyncing = syncing;
        emit syncingChanged();
    }
}

// 同步操作实现
void BaseSyncServer::与服务器同步(SyncDirection direction) {
    if (!是否可以执行同步()) {
        emit syncCompleted(UnknownError, "无法同步");
        return;
    }
    qDebug() << "与服务器同步开始，当前同步状态:" << m_isSyncing;

    setIsSyncing(true);
    m_currentSyncDirection = direction;
    emit syncStarted();

    执行同步(direction);
}

// 默认的同步操作实现
void BaseSyncServer::重置同步状态() {
    m_isSyncing = false;
    m_currentSyncDirection = Bidirectional;
    emit syncingChanged();
}

void BaseSyncServer::取消同步() {
    if (m_isSyncing) {
        setIsSyncing(false);
        emit syncCompleted(UnknownError, "同步已取消");
    }
}

// 网络请求处理（默认实现）
void BaseSyncServer::onNetworkRequestCompleted( //
    [[maybe_unused]] Network::RequestType type, //
    [[maybe_unused]] const nlohmann::json &response) {
    // 子类应该重写此方法来处理具体的响应
}

void BaseSyncServer::onNetworkRequestFailed(    //
    [[maybe_unused]] Network::RequestType type, //
    Network::Error error, const std::string &message) {

    if (m_isSyncing) {
        setIsSyncing(false);

        SyncResult result = NetworkError;
        if (error == Network::Error::AuthenticationError) {
            result = AuthError;
        }

        emit syncCompleted(result, QString::fromStdString(message));
    }
}

void BaseSyncServer::onAutoSyncTimer() {
    if (是否可以执行同步()) {
        与服务器同步();
    }
}

// 同步操作实现 - 重写基类虚函数
void BaseSyncServer::执行同步(SyncDirection direction) {
    qDebug() << "开始同步待办事项，方向:" << direction;

    // 根据同步方向执行不同的操作
    switch (direction) {
    case Bidirectional:
        拉取数据();
        break;
    case UploadOnly:
        // 仅上传：只推送本地更改
        推送数据();
        break;
    case DownloadOnly:
        // 仅下载：只获取服务器数据
        拉取数据();
        break;
    }
}

void BaseSyncServer::更新最后同步时间() {
    m_lastSyncTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

bool BaseSyncServer::是否可以执行同步() const {
    // 检查是否可以执行同步
    if (m_isSyncing) {
        qDebug() << "同步检查失败：正在进行同步操作，当前同步状态:" << m_isSyncing;
        // qDebug() << "提示：如果同步状态异常，请调用resetSyncState()方法重置";
        return false;
    }

    if (m_networkRequest.getServerBaseUrl().empty()) {
        qDebug() << "同步检查失败：服务器基础URL为空";
        return false;
    }

    if (m_apiEndpoint.empty()) {
        qDebug() << "同步检查失败：API端点为空";
        return false;
    }

    // 检查用户登录状态
    if (!m_userAuth.是否已登录()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        return false;
    }

    return true;
}

void BaseSyncServer::开启自动同步计时器() {
    if (m_autoSyncTimer && m_autoSyncInterval > 0) {
        m_autoSyncTimer->start(m_autoSyncInterval * 60 * 1000); // 转换为毫秒
    }
}

void BaseSyncServer::停止自动同步计时器() {
    if (m_autoSyncTimer) {
        m_autoSyncTimer->stop();
    }
}
