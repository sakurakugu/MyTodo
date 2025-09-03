/**
 * @file BaseDialog.qml
 * @brief 统一的弹窗基础组件
 *
 * 提供所有弹窗的基础样式、属性和行为，确保整个应用程序中
 * 弹窗的一致性和可维护性。所有自定义弹窗都应该继承此组件。
 *
 * @author Sakurakugu
 * @date 2025-08-23 16:13:21(UTC+8) 周六
 * @change 2025-09-03 13:07:00(UTC+8) 周三
 * @version 1.0.0
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

/**
 * @brief 弹窗基础组件
 *
 * 提供统一的弹窗样式、动画效果和通用属性。
 * 所有自定义弹窗都应该继承此组件以保持一致性。
 */
Dialog {
    id: root

    // 公共属性
    property string dialogTitle: ""                       ///< 对话框标题
    property int dialogWidth: 300                         ///< 对话框最小宽度
    property int dialogHeight: 200                        ///< 对话框最小高度
    property int maxDialogWidth: 800                      ///< 对话框最大宽度
    property int maxDialogHeight: 600                     ///< 对话框最大高度
    property bool autoSize: true                          ///< 是否根据内容自动调整大小
    property alias contentLayout: contentColumn           ///< 内容布局别名
    property int contentMargins: 20                       ///< 内容边距
    property int contentSpacing: 15                       ///< 内容间距
    property bool enableAnimation: true                   ///< 是否启用动画
    property real animationDuration: 200                  ///< 动画持续时间
    readonly property var theme: ThemeManager             ///< 主题
    property string confirmText: qsTr("确定")             ///< 确定按钮文本
    property string cancelText: qsTr("取消")              ///< 取消按钮文本

    property bool isShowConfirmButton: true              ///< 确定按钮是否显示
    property bool isShowCancelButton: true               ///< 取消按钮是否显示

    property bool isEnableConfirmButton: true            ///< 确定按钮是否可用
    property bool isEnableCancelButton: true             ///< 取消按钮是否可用

    // 对话框基本设置
    title: dialogTitle
    anchors.centerIn: parent
    modal: true

    // 动态尺寸设置
    width: autoSize ? Math.max(dialogWidth, Math.min(maxDialogWidth, contentColumn.implicitWidth + contentMargins * 2)) : dialogWidth
    height: autoSize ? Math.max(dialogHeight, Math.min(maxDialogHeight, contentColumn.implicitHeight + contentMargins * 2 + (header ? header.height : 0) + (footer ? footer.height : 0))) : dialogHeight

    // 自定义两个信号
    signal confirmed    ///< 确定信号
    signal cancelled    ///< 取消信号

    // 标题栏
    header: Rectangle {
        color: root.theme.backgroundColor
        border.color: root.theme.borderColor
        height: root.dialogTitle !== "" ? 40 : 0
        visible: root.dialogTitle !== ""
        topLeftRadius: 10                                     ///< 左上角圆角
        topRightRadius: 10                                    ///< 右上角圆角

        Text {
            text: root.dialogTitle
            color: root.theme.textColor
            font.pixelSize: 16
            font.bold: true
            anchors.centerIn: parent
        }

        // 底部分割线
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: root.theme.borderColor
        }
    }

    // 背景
    background: Rectangle {
        color: root.theme.backgroundColor
        border.color: root.theme.borderColor
        border.width: 1
        radius: 10

        // 添加阴影效果
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 2
            shadowBlur: 0.8
            shadowColor: ThemeManager.shadowColor
        }
    }

    // 内容区域
    contentItem: ColumnLayout {
        id: contentColumn
        spacing: root.contentSpacing
        anchors.margins: root.contentMargins
        focus: true                                          ///< 获得焦点
        Keys.enabled: true                                  ///< 启用键盘事件

        // 支持自动尺寸调整
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                root.cancelled();
                console.info("按下Esc按键")
            } else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                root.confirmed();
                console.info("按下Enter按键")
            }
        }

        // 子类可以在这里添加自定义内容
    }

    // 按钮区域
    footer: RowLayout {
        Layout.fillWidth: true

        // 弹性空间
        Item {
            Layout.fillWidth: true
        }

        // 取消按钮
        Button {
            id: cancelButton
            text: root.cancelText
            visible: root.isShowCancelButton
            enabled: root.isEnableCancelButton
            Layout.rightMargin: root.isShowConfirmButton ? 15 : 20    // 右边距

            background: Rectangle {
                color: parent.enabled ? (parent.pressed ? root.theme.button2PressedColor : parent.hovered ? root.theme.button2HoverColor : root.theme.button2Color) : root.theme.button2DisabledColor
                border.color: root.theme.borderColor
                border.width: 1
                radius: 8
            }

            contentItem: Text {
                text: parent.text
                color: parent.enabled ? root.theme.button2TextColor : root.theme.button2DisabledTextColor
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.cancelled()
        }

        // 确定按钮
        Button {
            id: confirmButton
            text: root.confirmText
            visible: root.isShowConfirmButton
            enabled: root.isEnableConfirmButton
            Layout.rightMargin: 20

            background: Rectangle {
                color: parent.enabled ? (parent.pressed ? root.theme.buttonPressedColor : parent.hovered ? root.theme.buttonHoverColor : root.theme.buttonColor) : root.theme.buttonDisabledColor
                border.color: root.theme.borderColor
                border.width: 1
                radius: 8
            }

            contentItem: Text {
                text: parent.text
                color: parent.enabled ? root.theme.buttonTextColor : root.theme.buttonDisabledTextColor
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.confirmed()
        }
    }

    // 进入动画
    enter: Transition {
        enabled: root.enableAnimation

        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: root.animationDuration
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                property: "scale"
                from: 0.8
                to: 1.0
                duration: root.animationDuration
                easing.type: Easing.OutBack
                easing.overshoot: 1.2
            }
        }
    }

    // 退出动画
    exit: Transition {
        enabled: root.enableAnimation

        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: root.animationDuration * 0.75
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: root.animationDuration * 0.75
                easing.type: Easing.InQuad
            }
        }
    }

    /**
     * @brief 设置对话框尺寸
     * @param w 宽度
     * @param h 高度
     * @param enableAutoSize 是否启用自动尺寸调整
     */
    function setSize(w, h, enableAutoSize) {
        root.dialogWidth = w;
        root.dialogHeight = h;
        if (enableAutoSize !== undefined) {
            root.autoSize = enableAutoSize;
        }
    }

    /**
     * @brief 设置最大尺寸限制
     * @param maxWidth 最大宽度
     * @param maxHeight 最大高度
     */
    function setMaxSize(maxWidth, maxHeight) {
        root.maxDialogWidth = maxWidth;
        root.maxDialogHeight = maxHeight;
    }

    /**
     * @brief 设置按钮文本
     * @param confirmText 确定按钮文本
     * @param cancelText 取消按钮文本
     */
    function setButtonTexts(confirmText, cancelText) {
        if (confirmText !== undefined)
            root.confirmText = confirmText;
        if (cancelText !== undefined)
            root.cancelText = cancelText;
    }

    /**
     * @brief 设置按钮状态
     * @param isShowCancel 取消按钮是否可见
     * @param isShowConfirm 确定按钮是否可见
     */
    function setButtonVisible(isShowConfirm, isShowCancel) {
        if (isShowCancel !== undefined)
            root.isShowCancelButton = isShowCancel;
        if (isShowConfirm !== undefined)
            root.isShowConfirmButton = isShowConfirm;
    }
}
