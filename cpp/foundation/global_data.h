/**
 * @file global_data.h
 * @brief GlobalData类的头文件
 *
 * 该文件定义了GlobalData类，用于管理应用程序的全局状态。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-10-05 23:50:47(UTC+8) 周日
 */
#pragma once

#include <QObject>
#include <QVariant>

class Config;

class GlobalData {
  public:
    // 单例模式
    static GlobalData &GetInstance() {
        static GlobalData instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    GlobalData(const GlobalData &) = delete;
    GlobalData &operator=(const GlobalData &) = delete;
    GlobalData(GlobalData &&) = delete;
    GlobalData &operator=(GlobalData &&) = delete;

  signals:
  private:
    explicit GlobalData();
    ~GlobalData();

  private:
};
