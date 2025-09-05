import QtQuick
import QtQuick.Controls

/**
 * 分割线组件，适合用在Layout中，用在Rectangle中老是飘到顶部，还不如直接用如下注释掉的Rectangle
 * 提供水平和垂直分割线功能，支持自定义样式
 *
 * @author Sakurakugu
 */

//  Rectangle {
//     anchors.left: parent.left
//     anchors.right: parent.right
//     anchors.bottom: parent.bottom
//     color: ThemeManager.borderColor
//     height: 1
// }

Item {
    id: root

    // 公开属性
    property int orientation: Qt.Horizontal        ///< 分割线方向：Qt.Horizontal（水平）或 Qt.Vertical（垂直）
    property bool isTopOrLeft: false                //< 分割线是否在顶部（水平）或左侧（垂直）
    property color color: ThemeManager.borderColor ///< 分割线颜色
    property int thickness: 1                      ///< 分割线厚度
    property int leftMargin: 0                     ///< 左边距
    property int rightMargin: 0                    ///< 右边距
    property int topMargin: 0                      ///< 上边距
    property int bottomMargin: 0                   ///< 下边距

    // 根据方向设置默认尺寸
    implicitWidth: orientation === Qt.Horizontal ? parent.width : thickness
    implicitHeight: orientation === Qt.Horizontal ? thickness : parent.height

    // 分割线矩形
    Rectangle {
        id: line
        color: root.color

        // 根据方向设置位置和尺寸
        anchors {
            left: root.orientation === Qt.Vertical ? (root.isTopOrLeft ? parent.left : undefined) : (parent.left)
            right: root.orientation === Qt.Horizontal ? (parent.right) : (root.isTopOrLeft ? undefined : parent.right)
            top: root.orientation === Qt.Vertical ? (parent.top) : (root.isTopOrLeft ? parent.top : undefined)
            bottom: root.orientation === Qt.Horizontal ? (root.isTopOrLeft ? undefined : parent.bottom) : (parent.bottom)

            leftMargin: root.leftMargin
            rightMargin: root.rightMargin
            topMargin: root.topMargin
            bottomMargin: root.bottomMargin
        }

        width: root.orientation === Qt.Vertical ? root.thickness : undefined
        height: root.orientation === Qt.Horizontal ? root.thickness : undefined
    }
}
