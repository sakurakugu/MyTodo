/**
 * @file WindowDragHandler.qml
 * @brief 窗口拖拽处理组件
 *
 * 提供统一的窗口拖拽功能，支持：
 * - 普通窗口拖拽
 * - 最大化窗口拖拽恢复
 * - 小组件模式拖拽控制
 * - 智能位置计算
 *
 * @author Sakurakugu
 * @date 2025-08-31 22:44:07(UTC+8) 周日
 * @change 2025-09-05 23:00:06(UTC+8) 周五
 */

import QtQuick

/**
 * @brief 窗口拖拽处理组件
 *
 * 封装窗口拖拽逻辑的MouseArea组件，可直接嵌入到需要拖拽功能的区域中。
 * 支持最大化状态下的智能拖拽恢复和小组件模式的拖拽控制。
 */
MouseArea {
    // 必需属性 - 需要外部传入
    property Window targetWindow: null              ///< 目标窗口对象

    // 内部状态属性
    property point clickPos: Qt.point(0, 0)        ///< 记录鼠标按下时的位置
    property bool wasMaximized: false              ///< 记录拖拽开始时是否为最大化状态

    // 信号
    signal dragStarted                           ///< 拖拽开始信号
    signal dragFinished                          ///< 拖拽结束信号
    signal windowRestored                        ///< 窗口从最大化恢复信号

    // 确保此MouseArea在其他控件下层，不影响其他控件的点击事件
    z: -1

    /**
     * @brief 鼠标按下事件处理
     *
     * 记录鼠标按下位置和窗口状态，为拖拽做准备。
     *
     * @param mouse 鼠标事件对象
     */
    onPressed: function (mouse) {
        if (!targetWindow) {
            // console.warn(qsTr("未设置目标窗口，无法处理拖拽事件"));
            return;
        }

        clickPos = Qt.point(mouse.x, mouse.y);
        wasMaximized = (targetWindow.visibility === Window.Maximized);
        dragStarted();
    }

    /**
     * @brief 鼠标移动事件处理 - 实现窗口拖拽
     *
     * 根据不同状态执行相应的拖拽逻辑：
     * - 最大化状态：智能恢复到合适位置
     * - 普通状态：直接拖拽移动
     * - 小组件模式：根据preventDragging属性决定是否允许拖拽
     *
     * @param mouse 鼠标事件对象
     */
    onPositionChanged: function (mouse) {
        if (!targetWindow) {
            return;
        }

        // 只有在非小组件模式或小组件模式但未启用防止拖动时才允许拖动
        if (pressed && ((!globalData.isDesktopWidget) || (globalData.isDesktopWidget && !globalData.preventDragging))) {
            if (wasMaximized) {
                // 如果是从最大化状态开始拖拽，需要特殊处理
                handleMaximizedDrag(mouse);
            } else {
                // 普通拖拽逻辑
                handleNormalDrag(mouse);
            }
        }
    }

    /**
     * @brief 鼠标释放事件处理
     *
     * 拖拽结束时的清理工作。
     */
    onReleased: function () {
        dragFinished();
    }

    /**
     * @brief 处理最大化状态下的拖拽
     *
     * 当从最大化状态开始拖拽时，需要：
     * 1. 计算鼠标在标题栏中的相对位置
     * 2. 恢复窗口到正常状态
     * 3. 根据相对位置调整窗口位置，保持鼠标在相同的相对位置
     *
     * @param mouse 鼠标事件对象
     */
    function handleMaximizedDrag(mouse) {
        // 计算鼠标在标题栏中的相对位置
        var mouseRatioX = clickPos.x / width;

        // 恢复窗口到正常状态
        targetWindow.showNormal();
        windowRestored();

        // 根据鼠标相对位置调整窗口位置，使鼠标保持在相同的相对位置
        var globalMousePos = mapToGlobal(mouse.x, mouse.y);
        targetWindow.x = globalMousePos.x - (targetWindow.width * mouseRatioX);
        targetWindow.y = globalMousePos.y - mouse.y;

        // 更新点击位置为新窗口大小下的相对位置
        clickPos = Qt.point(targetWindow.width * mouseRatioX, mouse.y);
        wasMaximized = false;
    }

    /**
     * @brief 处理普通状态下的拖拽
     *
     * 计算鼠标移动的偏移量，直接应用到窗口位置上。
     *
     * @param mouse 鼠标事件对象
     */
    function handleNormalDrag(mouse) {
        var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
        targetWindow.x += delta.x;
        targetWindow.y += delta.y;
    }

    /**
     * @brief 设置目标窗口
     *
     * 动态设置需要拖拽的目标窗口。
     *
     * @param window 目标窗口对象
     */
    function setTargetWindow(window) {
        targetWindow = window;
    }

    /**
     * @brief 设置小组件模式
     *
     * 配置小组件模式相关属性。
     *
     * @param isWidget 是否为小组件模式
     * @param preventDrag 是否阻止拖拽
     */
    function setWidgetMode(isWidget, preventDrag) {
        globalData.isDesktopWidget = isWidget;
        if (preventDrag !== undefined) {
            globalData.preventDragging = preventDrag;
        }
    }

    /**
     * @brief 重置拖拽状态
     *
     * 重置所有内部状态，用于异常情况下的状态清理。
     */
    function resetState() {
        clickPos = Qt.point(0, 0);
        wasMaximized = false;
    }
}
