#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include "../cpp/todomodel.h"
#include "../cpp/todoitem.h"

class TestTodoModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testAddTodo();
    void testRemoveTodo();
    void testUpdateTodo();
    void testToggleCompleted();
    void testClearCompleted();
    void testDataMethod();
    void testRoleNames();
    void testRowCount();
    void testSignalEmission();
    void cleanupTestCase();

private:
    TodoModel* model;
};

void TestTodoModel::initTestCase()
{
    model = new TodoModel(this);
    // 清理本地存储，确保测试从干净状态开始
    while (model->rowCount() > 0) {
        model->removeTodo(0);
    }
}

void TestTodoModel::testInitialState()
{
    QCOMPARE(model->rowCount(), 0);
    QVERIFY(model->roleNames().contains(TodoModel::TitleRole));
    QVERIFY(model->roleNames().contains(TodoModel::DescriptionRole));
    QVERIFY(model->roleNames().contains(TodoModel::CategoryRole));
    QVERIFY(model->roleNames().contains(TodoModel::UrgencyRole));
    QVERIFY(model->roleNames().contains(TodoModel::ImportanceRole));
    QVERIFY(model->roleNames().contains(TodoModel::StatusRole));
}

void TestTodoModel::testAddTodo()
{
    QSignalSpy rowsInsertedSpy(model, &QAbstractListModel::rowsInserted);
    
    int initialCount = model->rowCount();
    
    model->addTodo("测试任务", "测试描述", "工作", "高", "中");
    
    QCOMPARE(model->rowCount(), initialCount + 1);
    QCOMPARE(rowsInsertedSpy.count(), 1);
    
    // 验证添加的数据
    QModelIndex index = model->index(model->rowCount() - 1);
    QCOMPARE(model->data(index, TodoModel::TitleRole).toString(), QString("测试任务"));
    QCOMPARE(model->data(index, TodoModel::DescriptionRole).toString(), QString("测试描述"));
    QCOMPARE(model->data(index, TodoModel::CategoryRole).toString(), QString("工作"));
    QCOMPARE(model->data(index, TodoModel::UrgencyRole).toString(), QString("高"));
    QCOMPARE(model->data(index, TodoModel::ImportanceRole).toString(), QString("中"));
    QCOMPARE(model->data(index, TodoModel::StatusRole).toString(), QString("todo"));
}

void TestTodoModel::testRemoveTodo()
{
    // 先添加一个任务
    model->addTodo("待删除任务", "描述", "工作", "中", "中");
    int initialCount = model->rowCount();
    
    QSignalSpy rowsRemovedSpy(model, &QAbstractListModel::rowsRemoved);
    
    // 删除最后一个任务
    model->removeTodo(initialCount - 1);
    
    QCOMPARE(model->rowCount(), initialCount - 1);
    QCOMPARE(rowsRemovedSpy.count(), 1);
}

void TestTodoModel::testUpdateTodo()
{
    // 先添加一个任务
    model->addTodo("原始标题", "原始描述", "工作", "高", "中");
    int lastIndex = model->rowCount() - 1;
    
    QSignalSpy dataChangedSpy(model, &QAbstractListModel::dataChanged);
    
    // 更新任务
    QVariantMap updateData;
    updateData["title"] = "更新标题";
    updateData["description"] = "更新描述";
    updateData["category"] = "学习";
    updateData["urgency"] = "低";
    updateData["importance"] = "高";
    model->updateTodo(lastIndex, updateData);
    
    QCOMPARE(dataChangedSpy.count(), 1);
    
    // 验证更新的数据
    QModelIndex index = model->index(lastIndex);
    QCOMPARE(model->data(index, TodoModel::TitleRole).toString(), QString("更新标题"));
    QCOMPARE(model->data(index, TodoModel::DescriptionRole).toString(), QString("更新描述"));
    QCOMPARE(model->data(index, TodoModel::CategoryRole).toString(), QString("学习"));
    QCOMPARE(model->data(index, TodoModel::UrgencyRole).toString(), QString("低"));
    QCOMPARE(model->data(index, TodoModel::ImportanceRole).toString(), QString("高"));
}

void TestTodoModel::testToggleCompleted()
{
    // 先添加一个任务
    model->addTodo("切换完成状态", "描述", "工作", "中", "中");
    int lastIndex = model->rowCount() - 1;
    
    QSignalSpy dataChangedSpy(model, &QAbstractListModel::dataChanged);
    
    // 初始状态应该是待办
    QModelIndex index = model->index(lastIndex);
    QCOMPARE(model->data(index, TodoModel::StatusRole).toString(), QString("todo"));
    
    // 标记为完成
    model->markAsDone(lastIndex);
    QCOMPARE(model->data(index, TodoModel::StatusRole).toString(), QString("done"));
    QCOMPARE(dataChangedSpy.count(), 1);
    
    // 验证数据变化信号被正确发出
    QVERIFY(dataChangedSpy.count() > 0);
}

void TestTodoModel::testClearCompleted()
{
    // 添加几个任务，其中一些标记为完成
    model->addTodo("任务1", "描述1", "工作", "高", "中");
    model->addTodo("任务2", "描述2", "学习", "中", "高");
    model->addTodo("任务3", "描述3", "生活", "低", "低");
    
    int task1Index = model->rowCount() - 3;
    int task3Index = model->rowCount() - 1;
    
    // 标记任务1和任务3为完成
    model->markAsDone(task1Index);
    model->markAsDone(task3Index);
    
    // 验证任务已标记为完成
    QModelIndex index1 = model->index(task1Index, 0);
    QModelIndex index3 = model->index(task3Index, 0);
    QCOMPARE(model->data(index1, TodoModel::StatusRole).toString(), QString("done"));
    QCOMPARE(model->data(index3, TodoModel::StatusRole).toString(), QString("done"));
    
    // 验证未完成的任务状态
    QModelIndex index2 = model->index(task1Index + 1, 0);
    QCOMPARE(model->data(index2, TodoModel::StatusRole).toString(), QString("todo"));
}

void TestTodoModel::testDataMethod()
{
    model->addTodo("数据测试", "测试data方法", "工作", "高", "中");
    int lastIndex = model->rowCount() - 1;
    QModelIndex index = model->index(lastIndex);
    
    // 测试有效角色
    QVERIFY(!model->data(index, TodoModel::TitleRole).toString().isEmpty());
    QVERIFY(!model->data(index, TodoModel::DescriptionRole).toString().isEmpty());
    QVERIFY(!model->data(index, TodoModel::CategoryRole).toString().isEmpty());
    QVERIFY(!model->data(index, TodoModel::UrgencyRole).toString().isEmpty());
    QVERIFY(!model->data(index, TodoModel::ImportanceRole).toString().isEmpty());
    QVERIFY(model->data(index, TodoModel::StatusRole).isValid());
    
    // 测试无效角色
    QVERIFY(!model->data(index, 9999).isValid());
    
    // 测试无效索引
    QModelIndex invalidIndex = model->index(-1);
    QVERIFY(!model->data(invalidIndex, TodoModel::TitleRole).isValid());
}

void TestTodoModel::testRoleNames()
{
    QHash<int, QByteArray> roles = model->roleNames();
    
    QCOMPARE(roles[TodoModel::TitleRole], QByteArray("title"));
    QCOMPARE(roles[TodoModel::DescriptionRole], QByteArray("description"));
    QCOMPARE(roles[TodoModel::CategoryRole], QByteArray("category"));
    QCOMPARE(roles[TodoModel::UrgencyRole], QByteArray("urgency"));
    QCOMPARE(roles[TodoModel::ImportanceRole], QByteArray("importance"));
    QCOMPARE(roles[TodoModel::StatusRole], QByteArray("status"));
}

void TestTodoModel::testRowCount()
{
    int initialCount = model->rowCount();
    
    model->addTodo("计数测试1", "描述", "工作", "高", "中");
    QCOMPARE(model->rowCount(), initialCount + 1);
    
    model->addTodo("计数测试2", "描述", "学习", "中", "高");
    QCOMPARE(model->rowCount(), initialCount + 2);
    
    model->removeTodo(model->rowCount() - 1);
    QCOMPARE(model->rowCount(), initialCount + 1);
}

void TestTodoModel::testSignalEmission()
{
    QSignalSpy rowsInsertedSpy(model, &QAbstractListModel::rowsInserted);
    QSignalSpy rowsRemovedSpy(model, &QAbstractListModel::rowsRemoved);
    QSignalSpy dataChangedSpy(model, &QAbstractListModel::dataChanged);
    
    // 测试添加信号
    model->addTodo("信号测试", "描述", "工作", "高", "中");
    QCOMPARE(rowsInsertedSpy.count(), 1);
    
    int lastIndex = model->rowCount() - 1;
    
    // 测试数据变化信号
    model->markAsDone(lastIndex);
    QCOMPARE(dataChangedSpy.count(), 1);
    
    // 测试删除信号
    model->removeTodo(lastIndex);
    QCOMPARE(rowsRemovedSpy.count(), 1);
}

void TestTodoModel::cleanupTestCase()
{
    delete model;
    model = nullptr;
}

QTEST_MAIN(TestTodoModel)
#include "test_todomodel.moc"