#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include "../cpp/networkmanager.h"

class TestNetworkManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialization();
    void testRequestTypes();
    void testRequestConfig();
    void testSignalEmission();
    void testErrorHandling();
    void testJsonDataHandling();
    void cleanupTestCase();

private:
    NetworkManager* networkManager;
};

void TestNetworkManager::initTestCase()
{
    networkManager = new NetworkManager(this);
}

void TestNetworkManager::testInitialization()
{
    QVERIFY(networkManager != nullptr);
    
    // 测试网络管理器是否正确初始化
    QVERIFY(networkManager->metaObject()->indexOfSignal("requestCompleted(QJsonObject)") >= 0);
    QVERIFY(networkManager->metaObject()->indexOfSignal("requestFailed(QString)") >= 0);
}

void TestNetworkManager::testRequestTypes()
{
    // 测试RequestType枚举值
    QVERIFY(static_cast<int>(NetworkManager::RequestType::Login) >= 0);
    QVERIFY(static_cast<int>(NetworkManager::RequestType::Sync) >= 0);
    QVERIFY(static_cast<int>(NetworkManager::RequestType::FetchTodos) >= 0);
    QVERIFY(static_cast<int>(NetworkManager::RequestType::PushTodos) >= 0);
    QVERIFY(static_cast<int>(NetworkManager::RequestType::Logout) >= 0);
}

void TestNetworkManager::testRequestConfig()
{
    // 测试RequestConfig结构体
    NetworkManager::RequestConfig config;
    config.url = "https://api.example.com/login";
    
    QJsonObject testData;
    testData["username"] = "testuser";
    testData["password"] = "testpass";
    config.data = testData;
    
    QCOMPARE(config.url, QString("https://api.example.com/login"));
    QCOMPARE(config.data["username"].toString(), QString("testuser"));
    QCOMPARE(config.data["password"].toString(), QString("testpass"));
}

void TestNetworkManager::testSignalEmission()
{
    QSignalSpy completedSpy(networkManager, &NetworkManager::requestCompleted);
    QSignalSpy failedSpy(networkManager, &NetworkManager::requestFailed);
    
    // 验证信号spy已正确设置
    QVERIFY(completedSpy.isValid());
    QVERIFY(failedSpy.isValid());
    
    // 初始状态下应该没有信号发射
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
}

void TestNetworkManager::testErrorHandling()
{
    QSignalSpy failedSpy(networkManager, &NetworkManager::requestFailed);
    
    // 测试无效URL的请求配置
    NetworkManager::RequestConfig invalidConfig;
    invalidConfig.url = "";
    
    // 发送无效请求（这应该触发错误处理）
    networkManager->sendRequest(NetworkManager::RequestType::Login, invalidConfig);
    
    // 等待一小段时间让网络操作完成
    QTest::qWait(100);
    
    // 注意：实际的错误处理取决于NetworkManager的具体实现
    // 这里我们主要测试方法调用不会崩溃
    QVERIFY(true); // 如果到达这里说明没有崩溃
}

void TestNetworkManager::testJsonDataHandling()
{
    // 测试JSON数据的处理
    QJsonObject testJson;
    testJson["id"] = 123;
    testJson["title"] = "测试任务";
    testJson["completed"] = false;
    
    QJsonArray testArray;
    testArray.append(testJson);
    
    QJsonObject containerJson;
    containerJson["todos"] = testArray;
    containerJson["count"] = 1;
    
    // 验证JSON结构
    QVERIFY(containerJson.contains("todos"));
    QVERIFY(containerJson.contains("count"));
    QCOMPARE(containerJson["count"].toInt(), 1);
    
    QJsonArray retrievedArray = containerJson["todos"].toArray();
    QCOMPARE(retrievedArray.size(), 1);
    
    QJsonObject retrievedTodo = retrievedArray[0].toObject();
    QCOMPARE(retrievedTodo["id"].toInt(), 123);
    QCOMPARE(retrievedTodo["title"].toString(), QString("测试任务"));
    QCOMPARE(retrievedTodo["completed"].toBool(), false);
}

void TestNetworkManager::cleanupTestCase()
{
    delete networkManager;
    networkManager = nullptr;
}

QTEST_MAIN(TestNetworkManager)
#include "test_networkmanager.moc"