/**
 * @file qml_setting.cpp
 * @brief QmlSetting 实现（目前大部分为内联转发，留空以便未来扩展）
 *
 * 该文件实现了QmlSetting类，负责管理应用程序的设置。
 *
 * @author Sakurakugu
 * @date 2025-10-06 01:32:19(UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */
#include "qml_setting.h"

QmlSetting::QmlSetting(QObject *parent) : QObject(parent) {
    // 透传底层信号
    QObject::connect(&Setting::GetInstance(), &Setting::baseUrlChanged, this, &QmlSetting::baseUrlChanged);
}

// ---- 直接数据存取 ----
void QmlSetting::save(const QString &key, const QVariant &value) {
    Setting::GetInstance().保存(key, value);
}

QVariant QmlSetting::get(const QString &key, const QVariant &defaultValue) const {
    return Setting::GetInstance().读取(key, defaultValue);
}

void QmlSetting::remove(const QString &key) {
    Setting::GetInstance().移除(key);
}

bool QmlSetting::contains(const QString &key) const {
    return Setting::GetInstance().包含(key);
}

void QmlSetting::clear() {
    Setting::GetInstance().清除所有();
}

// ---- 文件 / 路径 ----
bool QmlSetting::openConfigFilePath() const {
    return Setting::GetInstance().打开配置文件路径();
}

QString QmlSetting::getConfigFilePath() const {
    return Setting::GetInstance().获取配置文件路径();
}

// ---- JSON 导入导出 ----
bool QmlSetting::exportConfigToJsonFile(const QString &filePath) {
    return Setting::GetInstance().导出配置到JSON文件(filePath);
}

bool QmlSetting::importConfigFromJsonFile(const QString &filePath, bool replaceAll) {
    return Setting::GetInstance().导入配置从JSON文件(filePath, replaceAll);
}

bool QmlSetting::exportDatabaseToJsonFile(const QString &filePath) {
    return Setting::GetInstance().导出数据库到JSON文件(filePath);
}

bool QmlSetting::importDatabaseFromJsonFile(const QString &filePath, bool replaceAll) {
    return Setting::GetInstance().导入数据库从JSON文件(filePath, replaceAll);
}

// ---- 配置文件位置管理与迁移 ----
int QmlSetting::getConfigLocation() const {
    return Setting::GetInstance().获取配置文件位置();
}

QString QmlSetting::getConfigLocationPath(int location) const {
    return Setting::GetInstance().获取指定配置位置路径(location);
}

bool QmlSetting::migrateConfigLocation(int targetLocation, bool overwriteExisting) {
    return Setting::GetInstance().迁移配置文件到位置(targetLocation, overwriteExisting);
}

// ---- 代理配置 ----
void QmlSetting::setProxyType(int type) {
    Setting::GetInstance().设置代理类型(type);
}

int QmlSetting::getProxyType() const {
    return Setting::GetInstance().获取代理类型();
}

void QmlSetting::setProxyHost(const QString &host) {
    Setting::GetInstance().设置代理主机(host);
}

QString QmlSetting::getProxyHost() const {
    return Setting::GetInstance().获取代理主机();
}

void QmlSetting::setProxyPort(int port) {
    Setting::GetInstance().设置代理端口(port);
}

int QmlSetting::getProxyPort() const {
    return Setting::GetInstance().获取代理端口();
}

void QmlSetting::setProxyUsername(const QString &username) {
    Setting::GetInstance().设置代理用户名(username);
}

QString QmlSetting::getProxyUsername() const {
    return Setting::GetInstance().获取代理用户名();
}

void QmlSetting::setProxyPassword(const QString &password) {
    Setting::GetInstance().设置代理密码(password);
}

QString QmlSetting::getProxyPassword() const {
    return Setting::GetInstance().获取代理密码();
}

void QmlSetting::setProxyEnabled(bool enabled) {
    Setting::GetInstance().设置代理是否启用(enabled);
}

bool QmlSetting::getProxyEnabled() const {
    return Setting::GetInstance().获取代理是否启用();
}

void QmlSetting::setProxyConfig(bool enableProxy, int type, const QString &host, int port, const QString &username,
                                const QString &password) {
    Setting::GetInstance().设置代理配置(enableProxy, type, host, port, username, password);
}

// ---- 服务器配置 ----
bool QmlSetting::isHttpsUrl(const QString &url) const {
    return Setting::GetInstance().是否是HttpsURL(url);
}

void QmlSetting::updateServerConfig(const QString &baseUrl) {
    Setting::GetInstance().更新服务器配置(baseUrl);
}

// ---- 自动备份配置 ----
void QmlSetting::setAutoBackupEnabled(bool enabled) {
    Setting::GetInstance().设置自动备份启用状态(enabled);
}

bool QmlSetting::getAutoBackupEnabled() const {
    return Setting::GetInstance().读取("backup/autoBackupEnabled", false).toBool();
}

void QmlSetting::setAutoBackupInterval(int days) {
    Setting::GetInstance().保存("backup/autoBackupInterval", days);
}

int QmlSetting::getAutoBackupInterval() const {
    return Setting::GetInstance().读取("backup/autoBackupInterval", 7).toInt(); // 默认7天
}

void QmlSetting::setAutoBackupPath(const QString &path) {
    Setting::GetInstance().保存("backup/autoBackupPath", path);
}

QString QmlSetting::getAutoBackupPath() const {
    return Setting::GetInstance().读取("backup/autoBackupPath", "").toString();
}

void QmlSetting::setMaxBackupFiles(int maxFiles) {
    Setting::GetInstance().保存("backup/maxBackupFiles", maxFiles);
}

int QmlSetting::getMaxBackupFiles() const {
    return Setting::GetInstance().读取("backup/maxBackupFiles", 5).toInt(); // 默认保留5个备份文件
}

bool QmlSetting::performBackup() {
    return Setting::GetInstance().执行备份();
}

QString QmlSetting::getLastBackupTime() const {
    return Setting::GetInstance().读取("backup/lastBackupTime", "").toString();
}

void QmlSetting::setLastBackupTime(const QString &time) {
    Setting::GetInstance().保存("backup/lastBackupTime", time);
}
