安装与使用
==========

1. 准备工作
-----------

C++20 标准带来了新的概念和协程等新特性，为了优化开发体验，Mirai++ 使用 C++20 标准进行编写，请先确认你的编译器支持 C++20 标准。

因为 Mirai++ 是基于 `mirai-api-http <https://github.com/project-mirai/mirai-api-http>`_ 工作的，所以你需要先安装好 `mirai-console <https://github.com/mamoe/mirai-console>`_ 以及 mirai-api-http 插件（推荐使用 `mirai-console-loader (mcl) <https://github.com/iTXTech/mirai-console-loader>`_ 安装）。具体安装流程可见上面给出的链接。

Mirai++ 是一个 C++ 语言的库，本身也有数个对其他 C++ 库的依赖，我采用了 `vcpkg <https://github.com/microsoft/vcpkg>`_ 做依赖管理。请先按链接中指导的过程安装好 vcpkg。手动安装并不是不可能，但因为过程会极其繁琐且易错这里强烈推荐采用 vcpkg。

Mirai++ 并不在 vcpkg 的主仓库中，所以并不能直接安装。你需要开启 vcpkg 的 registry 功能来安装第三方提供的库，而 registry 功能目前仍在预览阶段。若需要开启 vcpkg 的 registry 功能，请将环境变量 ``VCPKG_FEATURE_FLAGS`` 设为 ``registries,versions``。

2. 使用 Mirai++
---------------

2.1. 快速尝试 Mirai++
"""""""""""""""""""""

你可以克隆 `Mirai++ 项目模板 <https://github.com/Chlorie/miraipp-project-template>`_ 到本地快速开始开发（推荐！）。Mirai++ 采用 CMake 作为生成系统，最近的主流 C++ IDE (Visual Studio, Visual Studio Code, CLion) 都对 CMake 工程有直接的支持，并不需要单独地生成 IDE 特定的工程文件（如 VS 中的 .vcproj 等）。

VS 以及 VSC 都直接适配 vcpkg，若前面已经安装好 vcpkg 并且环境变量设置正确，打开项目以后 Mirai++ 的依赖就会自动开始安装。若要使用 CLion，请按照 `vcpkg <https://github.com/microsoft/vcpkg>`_ 文档中给出的步骤将 CMake toolchain 文件路径更改为 vcpkg 的 CMake toolchain 位置。

待所有依赖安装完成之后，编译项目即可。打开 ``src/main.cpp`` 就可以开始实验了。

或者，也直接克隆 `Mirai++ 的 git 主仓库 <https://github.com/Chlorie/miraipp>`_ 到本地，之后用前面说的方法用 IDE 打开并配置项目，之后就可以打开 ``examples`` 文件夹下的示例浏览 Mirai++ 支持的 API 和使用方法。

.. tip::
   建议只在需要尝试编译 Mirai++ 给出的范例的时候使用克隆主仓库的方法。尽量使用项目模板，或者如下一节中所说的那样用 vcpkg manifest 文件声明对 Mirai++ 的依赖，因为这样可以非常方便地进行自动更新。

.. _cmake use mpp:

2.2. 在自己的 CMake 项目中使用 Mirai++
"""""""""""""""""""""""""""""""""""""""

如果你想在已有的 CMake 项目中引入 Mirai++ 的话，只需要在项目根目录（包含 ``CMakeLists.txt`` 文件的最外层目录）中创建两个新的文件：``vcpkg.json`` 以及 ``vcpkg-configuration.json`` 列出对 Mirai++ 的依赖。

.. code-block:: js

   // vcpkg.json
   {
       "name": "项目的名称",
       "version-string": "版本号",
       // 列出项目的依赖
       "dependencies": [
           "miraipp"
       ]
   }

   // vcpkg-configuration.json
   {
       "registries": [
           // 引用我的第三方库，Mirai++ 依赖 clu 以及 libunifex
           {
               "kind": "git",
               "repository": "https://github.com/Chlorie/vcpkg-ports",
               "packages": [
                   "clu",
                   "miraipp",
                   "unifex"
               ]
           }
       ]
   }

要链接 Mirai++ 的话只需要在 ``CMakeLists.txt`` 中加入以下两行：

.. code-block:: cmake

   find_package(miraipp CONFIG REQUIRED)
   target_link_libraries(your_target PRIVATE miraipp::miraipp)

其实跟我给出的模板里面一样。可以参考一下我的 `项目模板 <https://github.com/Chlorie/miraipp-project-template>`_ 里面的写法。

2.3. 不使用 IDE 手动调用 CMake 安装
"""""""""""""""""""""""""""""""""""

配置好 vcpkg 及其环境变量之后，直接按照一般的 CMake 项目生成方式操作就行了，注意引用 vcpkg 的工具链文件（通常在 ``vcpkg/scripts/buildsystems/vcpkg.cmake``）。下面给出了典型的将 Mirai++ 编译为静态链接库并安装的指令：

.. code-block:: console

   $ git clone https://github.com/Chlorie/miraipp.git
   $ mkdir build
   $ cd build
   $ cmake .. -DCMAKE_TOOLCHAIN_FILE=/path-to-vcpkg-toolchain/vcpkg.cmake
   $ make
   $ sudo make install

若要编译动态链接库（shared library ``.so`` / dynamic library ``.dll``），在调用 CMake 的时候定义 ``BUILD_SHARED_LIBS`` 为 ``True``：

.. code-block:: console

   $ cmake .. -DCMAKE_TOOLCHAIN_FILE=/path-to-vcpkg-toolchain/vcpkg.cmake -DBUILD_SHARED_LIBS=True

或者也可以直接单独生成模板项目：

.. code-block:: console

   $ git clone https://github.com/Chlorie/miraipp-project-template.git
   $ mkdir build
   $ cd build
   $ cmake .. -DCMAKE_TOOLCHAIN_FILE=/path-to-vcpkg-toolchain/vcpkg.cmake
   $ make

3. 更新 Mirai++
---------------

非常简单，甚至根本不用浪费一节的版面来讲解。如果你使用了项目模板，或者使用了 `2.2 节 <cmake use mpp_>`_ 中提到的方法指出 Mirai++ 依赖，因为 vcpkg 自动整合 CMake，所以只要将你项目的 CMake 缓存重新 configure 一下（VS 的话在主 ``CMakeLists.txt`` 文件中按一下保存就会开始 configure），vcpkg 就会自动获取库依赖的更新。如果你是直接克隆的 Mirai++ 主仓库，运行 ``git pull`` 之后重新编译/生成即可。
