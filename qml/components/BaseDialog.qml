/**
 * @file BaseDialog.qml
 * @brief 统一的弹窗基础组件
 *
 * 提供所有弹窗的基础样式、属性和行为，确保整个应用程序中
 * 弹窗的一致性和可维护性。所有自定义弹窗都应该继承此组件。
 *
 * @author Sakurakugu
 * @date 2025-08-23 16:13:21(UTC+8) 周六
 * @version 2025-08-23 21:09:00(UTC+8) 周六
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
    id: baseDialog

    // 公共属性
    property bool isDarkMode: false                    ///< 深色模式开关
    property string dialogTitle: ""                    ///< 对话框标题
    property int dialogWidth: 300                      ///< 对话框最小宽度
    property int dialogHeight: 200                     ///< 对话框最小高度
    property int maxDialogWidth: 800                   ///< 对话框最大宽度
    property int maxDialogHeight: 600                  ///< 对话框最大高度
    property bool autoSize: true                       ///< 是否根据内容自动调整大小
    property bool showStandardButtons: true            ///< 是否显示标准按钮
    property int standardButtonFlags: Dialog.Ok | Dialog.Cancel  ///< 标准按钮类型
    property alias contentLayout: contentColumn        ///< 内容布局别名
    property int contentMargins: 20                    ///< 内容边距
    property int contentSpacing: 15                    ///< 内容间距
    property bool enableAnimation: true                ///< 是否启用动画
    property real animationDuration: 200               ///< 动画持续时间

    // 主题管理器
    ThemeManager {
        id: theme
        isDarkMode: baseDialog.isDarkMode
    }

    // 对话框基本设置
    title: dialogTitle
    modal: true
    anchors.centerIn: parent

    // 动态尺寸设置
    width: autoSize ? Math.max(dialogWidth, Math.min(maxDialogWidth, contentColumn.implicitWidth + contentMargins * 2)) : dialogWidth
    height: autoSize ? Math.max(dialogHeight, Math.min(maxDialogHeight, contentColumn.implicitHeight + contentMargins * 2 + (header ? header.height : 0) + (footer ? footer.height : 0))) : dialogHeight

    standardButtons: showStandardButtons ? standardButtonFlags : Dialog.NoButton

    // 自定义标题栏以支持主题颜色
    header: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        height: baseDialog.dialogTitle !== "" ? 40 : 0
        visible: baseDialog.dialogTitle !== ""
        topLeftRadius: 10                                     ///< 左上角圆角
        topRightRadius: 10                                    ///< 右上角圆角

        // 底部分割线
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: theme.borderColor
        }

        Text {
            text: baseDialog.dialogTitle
            color: theme.textColor
            font.pixelSize: 16
            font.bold: true
            anchors.centerIn: parent
        }
    }

    // 统一的背景样式
    background: Rectangle {
        color: theme.backgroundColor
        border.color: theme.borderColor
        border.width: 1
        radius: 10

        // 添加阴影效果
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 2
            shadowBlur: 0.8
            shadowColor: theme.shadowColor
        }
    }

    // 统一的内容区域
    contentItem: ColumnLayout {
        id: contentColumn
        spacing: baseDialog.contentSpacing
        anchors.margins: baseDialog.contentMargins

        // 支持自动尺寸调整
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight

        // 子类可以在这里添加自定义内容
    }

    // 统一的进入动画
    enter: Transition {
        enabled: baseDialog.enableAnimation

        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: baseDialog.animationDuration
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                property: "scale"
                from: 0.8
                to: 1.0
                duration: baseDialog.animationDuration
                easing.type: Easing.OutBack
                easing.overshoot: 1.2
            }
        }
    }

    // 统一的退出动画
    exit: Transition {
        enabled: baseDialog.enableAnimation

        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: baseDialog.animationDuration * 0.75
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: baseDialog.animationDuration * 0.75
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
        dialogWidth = w;
        dialogHeight = h;
        if (enableAutoSize !== undefined) {
            autoSize = enableAutoSize;
        }
    }

    /**
     * @brief 设置对话框标题
     * @param titleText 标题文本
     */
    function setTitle(titleText) {
        dialogTitle = titleText;
    }

    /**
     * @brief 启用或禁用自动尺寸调整
     * @param enable 是否启用自动尺寸调整
     */
    function setAutoSize(enable) {
        autoSize = enable;
    }

    /**
     * @brief 设置最大尺寸限制
     * @param maxWidth 最大宽度
     * @param maxHeight 最大高度
     */
    function setMaxSize(maxWidth, maxHeight) {
        maxDialogWidth = maxWidth;
        maxDialogHeight = maxHeight;
    }

    /**
     * @brief 设置最大高度限制
     * @param maxHeight 最大高度
     */
    function setMaxHeight(maxHeight) {
        maxDialogHeight = maxHeight;
    }
}
