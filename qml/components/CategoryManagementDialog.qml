import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: categoryDialog
    title: qsTr("类别管理")
    width: 400
    height: 500
    modal: true
    
    property alias themeManager: themeManager
    
    ThemeManager {
        id: themeManager
    }
    
    background: Rectangle {
        color: themeManager.isDarkMode ? "#2b2b2b" : "#ffffff"
        border.color: themeManager.isDarkMode ? "#555" : "#ccc"
        border.width: 1
        radius: 8
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // 标题
        Label {
            text: qsTr("类别管理")
            font.pixelSize: 18
            font.bold: true
            color: themeManager.isDarkMode ? "#ffffff" : "#333333"
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 添加新类别区域
        GroupBox {
            title: qsTr("添加新类别")
            Layout.fillWidth: true
            
            background: Rectangle {
                color: themeManager.isDarkMode ? "#3a3a3a" : "#f8f9fa"
                border.color: themeManager.isDarkMode ? "#555" : "#dee2e6"
                border.width: 1
                radius: 5
            }
            
            label: Label {
                text: parent.title
                color: themeManager.isDarkMode ? "#ffffff" : "#333333"
                font.pixelSize: 14
                font.bold: true
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                TextField {
                    id: newCategoryField
                    Layout.fillWidth: true
                    placeholderText: qsTr("输入新类别名称")
                    color: themeManager.isDarkMode ? "#ffffff" : "#333333"
                    
                    background: Rectangle {
                        color: themeManager.isDarkMode ? "#4a4a4a" : "#ffffff"
                        border.color: themeManager.isDarkMode ? "#666" : "#ccc"
                        border.width: 1
                        radius: 4
                    }
                }
                
                Button {
                    text: qsTr("添加")
                    enabled: newCategoryField.text.trim() !== ""
                    
                    background: Rectangle {
                        color: parent.enabled ? (themeManager.isDarkMode ? "#0d6efd" : "#007bff") : (themeManager.isDarkMode ? "#555" : "#ccc")
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#ffffff" : (themeManager.isDarkMode ? "#888" : "#666")
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        var categoryName = newCategoryField.text.trim()
                        if (categoryName !== "" && categoryName !== "全部") {
                            todoModel.createCategory(categoryName)
                            newCategoryField.text = ""
                        }
                    }
                }
            }
        }
        
        // 现有类别列表
        GroupBox {
            title: qsTr("现有类别")
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            background: Rectangle {
                color: themeManager.isDarkMode ? "#3a3a3a" : "#f8f9fa"
                border.color: themeManager.isDarkMode ? "#555" : "#dee2e6"
                border.width: 1
                radius: 5
            }
            
            label: Label {
                text: parent.title
                color: themeManager.isDarkMode ? "#ffffff" : "#333333"
                font.pixelSize: 14
                font.bold: true
            }
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 10
                
                ListView {
                    id: categoryListView
                    model: {
                        // 过滤掉"全部"类别，因为它不应该被删除
                        var filteredCategories = []
                        for (var i = 0; i < todoModel.categories.length; i++) {
                            if (todoModel.categories[i] !== "全部") {
                                filteredCategories.push(todoModel.categories[i])
                            }
                        }
                        return filteredCategories
                    }
                    
                    delegate: Rectangle {
                        width: categoryListView.width
                        height: 50
                        color: themeManager.isDarkMode ? "#4a4a4a" : "#ffffff"
                        border.color: themeManager.isDarkMode ? "#666" : "#dee2e6"
                        border.width: 1
                        radius: 4
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            
                            TextField {
                                id: categoryNameField
                                Layout.fillWidth: true
                                text: modelData
                                color: themeManager.isDarkMode ? "#ffffff" : "#333333"
                                
                                background: Rectangle {
                                    color: "transparent"
                                    border.color: categoryNameField.activeFocus ? (themeManager.isDarkMode ? "#0d6efd" : "#007bff") : "transparent"
                                    border.width: categoryNameField.activeFocus ? 2 : 0
                                    radius: 4
                                }
                                
                                onEditingFinished: {
                                    if (text.trim() !== "" && text.trim() !== modelData && text.trim() !== "全部") {
                                        todoModel.updateCategory(modelData, text.trim())
                                    } else {
                                        text = modelData // 恢复原值
                                    }
                                }
                            }
                            
                            Button {
                                text: qsTr("删除")
                                
                                background: Rectangle {
                                    color: themeManager.isDarkMode ? "#dc3545" : "#dc3545"
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    // 显示确认对话框
                                    confirmDeleteDialog.categoryToDelete = modelData
                                    confirmDeleteDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 底部按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            
            Button {
                text: qsTr("关闭")
                
                background: Rectangle {
                    color: themeManager.isDarkMode ? "#6c757d" : "#6c757d"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: categoryDialog.close()
            }
        }
    }
    
    // 删除确认对话框
    Dialog {
        id: confirmDeleteDialog
        title: qsTr("确认删除")
        width: 300
        height: 150
        modal: true
        
        property string categoryToDelete: ""
        
        background: Rectangle {
            color: themeManager.isDarkMode ? "#2b2b2b" : "#ffffff"
            border.color: themeManager.isDarkMode ? "#555" : "#ccc"
            border.width: 1
            radius: 8
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            Label {
                text: qsTr("确定要删除类别 \"%1\" 吗？").arg(confirmDeleteDialog.categoryToDelete)
                color: themeManager.isDarkMode ? "#ffffff" : "#333333"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                
                Button {
                    text: qsTr("取消")
                    
                    background: Rectangle {
                        color: themeManager.isDarkMode ? "#6c757d" : "#6c757d"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: confirmDeleteDialog.close()
                }
                
                Button {
                    text: qsTr("删除")
                    
                    background: Rectangle {
                        color: "#dc3545"
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        todoModel.deleteCategory(confirmDeleteDialog.categoryToDelete)
                        confirmDeleteDialog.close()
                    }
                }
            }
        }
    }
    
    // 监听类别操作完成信号
    Connections {
        target: todoModel
        function onCategoryOperationCompleted(success, message) {
            if (success) {
                console.log("类别操作成功:", message)
            } else {
                console.log("类别操作失败:", message)
                // 这里可以显示错误消息给用户
            }
        }
    }
}