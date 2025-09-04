import QtQuick
import QtQuick.Controls

/**
 * 分割线组件
 * 提供水平和垂直分割线功能，支持自定义样式
 */
Item {
    id: root

    // 公开属性
    property int orientation: Qt.Horizontal        ///< 分割线方向：Qt.Horizontal（水平）或 Qt.Vertical（垂直）
    property color color: ThemeManager.borderColor ///< 分割线颜色
    property int thickness: 1                      ///< 分割线厚度
    property int leftMargin: 0                     ///< 左边距
    property int rightMargin: 0                    ///< 右边距
    property int topMargin: 0                      ///< 上边距
    property int bottomMargin: 0                   ///< 下边距

    // 根据方向设置默认尺寸
    implicitWidth: orientation === Qt.Horizontal ? parent.width * 0.9 : thickness
    implicitHeight: orientation === Qt.Horizontal ? thickness : parent.height * 0.9

    // 分割线矩形
    Rectangle {
        id: line
        color: root.color
        // opacity: root.opacity

        // 根据方向设置位置和尺寸
        anchors {
            left: parent.left
            right: root.orientation === Qt.Horizontal ? parent.right : undefined
            top: parent.top
            bottom: root.orientation === Qt.Vertical ? parent.bottom : undefined

            leftMargin: root.leftMargin
            rightMargin: root.rightMargin
            topMargin: root.topMargin
            bottomMargin: root.bottomMargin
        }

        width: root.orientation === Qt.Vertical ? root.thickness : undefined
        height: root.orientation === Qt.Horizontal ? root.thickness : undefined
    }
}
