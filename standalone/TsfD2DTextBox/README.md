# TsfD2DTextBox

从 `TsfPad` 现有实现中抽出的独立 Win32 文本输入框控件原型，具备：

- Direct2D / DirectWrite 渲染
- TSF 文本服务接入
- 组合串显示属性绘制
- 软换行
- 基础选区、插入点、鼠标命中与候选框定位

## 目录结构

- `include/TsfD2DTextBox.h`
  公开头文件，宿主工程优先包含这个。
- `src/`
  控件内部实现与 TSF 支撑代码。
- `demo/TsfD2DTextBoxDemo.cpp`
  最小宿主示例。

## 当前形态

这是一版“可以单独编译和嵌入”的控件原型，不再依赖原来的 `TsfPad.cpp` 宿主窗口。

当前公开接口比较小：

- `TsfD2DTextBox::RegisterClass`
- `TsfD2DTextBox::Create`
- `TsfD2DTextBox::Move`
- `TsfD2DTextBox::GetHwnd`
- `TsfD2DTextBox::SetFont`

## 构建

仓库根目录的 CMake 已经加入这个子工程，目标包括：

- `TsfD2DTextBox`
- `TsfD2DTextBoxDemo`

示例可执行文件输出到：

- `build/bin/Debug/TsfD2DTextBoxDemo.exe`
