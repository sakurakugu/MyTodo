/**
 * @file config.h
 * @brief Config类的头文件
 *
 * 该文件定义了Config类，用于管理应用程序的本地配置文件。
 *
 * @author Sakurakugu
 * @date 2025
 */
#pragma once

#include <QObject>
#include <QSettings>
class Config : public QObject {
  public:
    // 单例模式
    static Config &GetInstance() {
        static Config instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

    bool save(const QString &key, const QVariant &value); ///< 保存设置到配置文件
    QVariant get(const QString &key,
                 const QVariant &defaultValue = QVariant()) const; ///< 从配置文件读取设置
    void remove(const QString &key);                               ///< 移除设置
    bool contains(const QString &key);                             ///< 检查设置是否存在
    QStringList allKeys();                                         ///< 获取所有设置的键名
    void clearSettings();                                          ///< 清除所有设置

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const;   ///< 打开配置文件所在目录
    QString getConfigFilePath() const; ///< 获取配置文件路径

  private:
    explicit Config(QObject *parent = nullptr);
    ~Config();

    QSettings *m_config; ///< 配置文件对象

    static const QSet<QString> booleanKeys; ///< 布尔类型的设置键名
};
