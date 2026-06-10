3D Model Viewer for .OBJ files
===============================

Visual Studio 2019 Win32 窗口程序
- 左侧: OpenGL 3D 渲染区域
- 右侧: GDI+ 深灰色风格交互面板（图形化快捷键界面）

-------
编译运行:

1. 双击打开 ModelViewer.sln (Visual Studio 2019)
2. 按 F5 编译并运行

-------
文件结构:

  ModelViewer.sln    - Visual Studio 2019 解决方案文件
  ModelViewer.vcxproj - Visual Studio 2019 项目文件
  main.cpp           - Win32 主程序 (OpenGL + GDI+)
  Model.h / Model.cpp - 3D 模型数据结构与操作
  Camera.h / Camera.cpp - 相机数据结构与操作
