/**
 * @file setting.h
 * @brief Setting类的头文件
 *
 * 该文件定义了Setting类，用于管理应用程序的配置信息。
 *
 * @author Sakurakugu
 * @date 2025
 */

#pragma once

#include <QObject>

class Setting : public QObject {
    Q_OBJECT
  public:
    static Setting &GetInstance() {
        static Setting instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    Setting(const Setting &) = delete;
    Setting &operator=(const Setting &) = delete;

    Q_INVOKABLE int getOsType() const;

  private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();
};
