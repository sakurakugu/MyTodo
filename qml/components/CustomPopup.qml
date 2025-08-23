/**
 * @file CustomPopup.qml
 * @brief 自定义弹窗组件
 *
 * 该文件定义了一个自定义弹窗组件，用于在应用程序中显示自定义内容的弹窗。
 * 弹窗可以包含标题、内容和关闭按钮，支持自定义样式和行为。
 *
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @version 2025-08-21 21:31:41(UTC+8) 周四
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: customPopup
    
    // 自定义属性
    property string title: ""
    property color backgroundColor: "white"
    property color borderColor: "#cccccc"
    property color titleColor: "black"
    property bool showCloseButton: true
    property int contentMargins: 10
    property alias contentLayout: contentColumn
    
    // 信号
    signal closeRequested()
    
    // 默认属性
    modal: false
    focus: true
    closePolicy: Popup.NoAutoClose
    
    // 内容区域
    contentItem: Rectangle {
        color: backgroundColor
        border.color: borderColor
        border.width: 1
        radius: 8
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: contentMargins
            spacing: 10
            
            // 标题栏
            RowLayout {
                Layout.fillWidth: true
                visible: title !== "" || showCloseButton
                
                Label {
                    text: title
                    font.bold: true
                    font.pixelSize: 16
                    color: titleColor
                    Layout.fillWidth: true
                }
                
                Button {
                    text: "✕"
                    flat: true
                    implicitWidth: 24
                    implicitHeight: 24
                    visible: showCloseButton
                    onClicked: {
                        customPopup.closeRequested()
                        customPopup.close()
                    }
                    
                    background: Rectangle {
                        color: "transparent"
                        radius: 12
                        border.width: parent.hovered ? 1 : 0
                        border.color: titleColor
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: titleColor
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            
            // 分割线
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                visible: title !== ""
            }
            
            // 内容区域
            ColumnLayout {
                id: contentColumn
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
            }
        }
    }
    
    // 添加进入和退出动画
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            duration: 200
            easing.type: Easing.OutQuad
        }
    }
    
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 150
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.8
            duration: 150
            easing.type: Easing.InQuad
        }
    }
}