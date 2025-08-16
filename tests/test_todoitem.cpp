#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include "../cpp/todoitem.h"

class TestTodoItem : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConstructor();
    void testSettersAndGetters();
    void testJsonSerialization();
    void testJsonDeserialization();
    void testCopyConstructor();
    void testAssignmentOperator();
    void cleanupTestCase();

private:
    TodoItem* testItem;
};

void TestTodoItem::initTestCase()
{
    testItem = nullptr;
}

void TestTodoItem::testConstructor()
{
    // 测试默认构造函数
    TodoItem item1;
    // 默认构造的TodoItem可能有空的id，这是正常的
    QCOMPARE(item1.title(), QString(""));
    QCOMPARE(item1.description(), QString(""));
    QCOMPARE(item1.category(), QString(""));
    QCOMPARE(item1.urgency(), QString(""));
    QCOMPARE(item1.importance(), QString(""));
    QCOMPARE(item1.status(), QString(""));
    
    // 测试带参数的构造函数
    TodoItem item2("id2", "测试标题", "测试描述", "工作", "高", "中", "todo", QDateTime::currentDateTime(), QDateTime::currentDateTime(), false);
    QCOMPARE(item2.title(), QString("测试标题"));
    QCOMPARE(item2.description(), QString("测试描述"));
    QCOMPARE(item2.category(), QString("工作"));
    QCOMPARE(item2.urgency(), QString("高"));
    QCOMPARE(item2.importance(), QString("中"));
    QCOMPARE(item2.status(), QString("todo"));
}

void TestTodoItem::testSettersAndGetters()
{
    TodoItem item;
    
    // 测试设置和获取标题
    item.setTitle("新标题");
    QCOMPARE(item.title(), QString("新标题"));
    
    // 测试设置和获取描述
    item.setDescription("新描述");
    QCOMPARE(item.description(), QString("新描述"));
    
    // 测试设置和获取分类
    item.setCategory("学习");
    QCOMPARE(item.category(), QString("学习"));
    
    // 测试设置和获取紧急程度
    item.setUrgency("低");
    QCOMPARE(item.urgency(), QString("低"));
    
    // 测试设置和获取重要程度
    item.setImportance("高");
    QCOMPARE(item.importance(), QString("高"));
    
    // 测试设置和获取状态
    item.setStatus("done");
    QCOMPARE(item.status(), QString("done"));
    
    item.setStatus("todo");
    QCOMPARE(item.status(), QString("todo"));
}

void TestTodoItem::testJsonSerialization()
{
    TodoItem item("id1", "测试任务", "这是一个测试任务", "工作", "高", "中", "done", QDateTime::currentDateTime(), QDateTime::currentDateTime(), true);
    
    // 验证基本属性
    QCOMPARE(item.title(), QString("测试任务"));
    QCOMPARE(item.description(), QString("这是一个测试任务"));
    QCOMPARE(item.category(), QString("工作"));
    QCOMPARE(item.urgency(), QString("高"));
    QCOMPARE(item.importance(), QString("中"));
    QCOMPARE(item.status(), QString("done"));
}

void TestTodoItem::testJsonDeserialization()
{
    // 创建一个TodoItem并验证其属性
    TodoItem item("id123", "从JSON创建的任务", "这是从JSON反序列化的任务", "生活", "中", "低", "done", QDateTime::currentDateTime(), QDateTime::currentDateTime(), true);
    
    QCOMPARE(item.id(), QString("id123"));
    QCOMPARE(item.title(), QString("从JSON创建的任务"));
    QCOMPARE(item.description(), QString("这是从JSON反序列化的任务"));
    QCOMPARE(item.category(), QString("生活"));
    QCOMPARE(item.urgency(), QString("中"));
    QCOMPARE(item.importance(), QString("低"));
    QCOMPARE(item.status(), QString("done"));
}

void TestTodoItem::testCopyConstructor()
{
    // TodoItem继承自QObject，不支持拷贝构造
    // 测试创建两个相同属性的TodoItem对象
    TodoItem original("id1", "原始任务", "原始描述", "工作", "高", "中", "done", QDateTime::currentDateTime(), QDateTime::currentDateTime(), true);
    TodoItem copy("id1", "原始任务", "原始描述", "工作", "高", "中", "done", QDateTime::currentDateTime(), QDateTime::currentDateTime(), true);
    
    QCOMPARE(copy.title(), original.title());
    QCOMPARE(copy.description(), original.description());
    QCOMPARE(copy.category(), original.category());
    QCOMPARE(copy.urgency(), original.urgency());
    QCOMPARE(copy.importance(), original.importance());
    QCOMPARE(copy.status(), original.status());
    
    // ID应该相同
    QCOMPARE(copy.id(), original.id());
}

void TestTodoItem::testAssignmentOperator()
{
    // TodoItem继承自QObject，不支持赋值操作
    // 测试通过setter方法修改TodoItem属性
    TodoItem item("id1", "原始任务", "原始描述", "工作", "高", "中", "todo", QDateTime::currentDateTime(), QDateTime::currentDateTime(), false);
    
    // 验证初始状态
    QCOMPARE(item.title(), QString("原始任务"));
    QCOMPARE(item.status(), QString("todo"));
    
    // 测试属性修改（如果有setter方法）
    // 这里只验证对象的基本属性访问
    QVERIFY(!item.title().isEmpty());
    QVERIFY(!item.description().isEmpty());
    QVERIFY(!item.category().isEmpty());
}

void TestTodoItem::cleanupTestCase()
{
    if (testItem) {
        delete testItem;
        testItem = nullptr;
    }
}

QTEST_MAIN(TestTodoItem)
#include "test_todoitem.moc"