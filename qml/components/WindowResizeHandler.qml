import QtQuick

/**
 * 窗口边框调整大小处理组件
 * 提供窗口边框和角落的拖拽调整大小功能
 */
Item {
    id: root
    z: 2000                              ///< 如果不起作用，就是z值太小了

    // 公开属性
    property int borderWidth: 5          ///< 边框调整大小的边距宽度
    property bool enabled: true          ///< 是否启用调整大小功能
    property Window targetWindow: null   ///< 目标窗口对象

    // 信号
    signal resizeStarted(int edges)      ///< 开始调整大小时发出的信号

    // 内部函数
    function startResize(edges) {
        if (targetWindow && enabled) {
            resizeStarted(edges);
            targetWindow.startSystemResize(edges);
        }
    }

    // 左边框
    MouseArea {
        id: leftResizeArea
        width: root.borderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.enabled
        cursorShape: Qt.SizeHorCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.LeftEdge);
        }
    }

    // 右边框
    MouseArea {
        id: rightResizeArea
        width: root.borderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.enabled
        cursorShape: Qt.SizeHorCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.RightEdge);
        }
    }

    // 上边框
    MouseArea {
        id: topResizeArea
        height: root.borderWidth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: root.enabled
        cursorShape: Qt.SizeVerCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.TopEdge);
        }
    }

    // 下边框
    MouseArea {
        id: bottomResizeArea
        height: root.borderWidth
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: root.enabled
        cursorShape: Qt.SizeVerCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.BottomEdge);
        }
    }

    // 左上角
    MouseArea {
        id: topLeftResizeArea
        width: root.borderWidth
        height: root.borderWidth
        anchors.left: parent.left
        anchors.top: parent.top
        visible: root.enabled
        cursorShape: Qt.SizeFDiagCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.TopEdge | Qt.LeftEdge);
        }
    }

    // 右上角
    MouseArea {
        id: topRightResizeArea
        width: root.borderWidth
        height: root.borderWidth
        anchors.right: parent.right
        anchors.top: parent.top
        visible: root.enabled
        cursorShape: Qt.SizeBDiagCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.TopEdge | Qt.RightEdge);
        }
    }

    // 左下角
    MouseArea {
        id: bottomLeftResizeArea
        width: root.borderWidth
        height: root.borderWidth
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: root.enabled
        cursorShape: Qt.SizeBDiagCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.BottomEdge | Qt.LeftEdge);
        }
    }

    // 右下角
    MouseArea {
        id: bottomRightResizeArea
        width: root.borderWidth
        height: root.borderWidth
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: root.enabled
        cursorShape: Qt.SizeFDiagCursor
        z: 2000

        onPressed: {
            root.startResize(Qt.BottomEdge | Qt.RightEdge);
        }
    }
}
