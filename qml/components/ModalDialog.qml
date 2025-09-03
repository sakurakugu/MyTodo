/**
 * @file ModalDialog.qml
 * @brief 模态对话框
 *
 * 用于显示模态对话框，用于提示用户输入或确认操作。
 *
 * @author Sakurakugu
 * @date 2025-09-03 07:39:54(UTC+8) 周二
 * @change 2025-09-03 21:09:00(UTC+8) 周六
 * @version 1.0.0
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 模态对话框
BaseDialog {
    id: root
    property string message: ""

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Label {
        text: qsTr(root.message)
        wrapMode: Text.WordWrap
        color: ThemeManager.textColor
        Layout.fillWidth: true
        Layout.topMargin: 10
        horizontalAlignment: Text.AlignHCenter
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    onCancelled: {
        root.close();
    }

    function showInfo(title, message) {
        root.dialogTitle = title;
        root.message = message;
        root.open();
    }

    function showMessage(message) {
        root.message = message;
        root.open();
    }
}
