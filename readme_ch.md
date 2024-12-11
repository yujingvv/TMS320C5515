# TMS320C5515 DSP 开发指南

## 如何安装 CCS TI 用于旧版 C55

1) 从 [TI 官网](https://www.ti.com/tool/CCSTUDIO) 下载并安装最新版本（或者从 [附加文件](https://github.com/lab-iu6/TMS320C5515/blob/setup/安装/ccs_setup_12.8.1.00005.exe) 下载）。

   在安装过程中，只需选择组件 **C55x ultra-low-power DSP**，如下图所示：
<div align="center">
  <img src="https://github.com/user-attachments/assets/7468cebd-9f54-4aa0-885b-90bf8de96808" alt="setup_components" width="800"/>
</div>

2) 下载并安装 [Code Generation Tools](https://github.com/lab-iu6/TMS320C5515/blob/setup/安装/ti_cgt_c5500_4.4.1_setup_win32.exe)（包含编译器、汇编器、链接器等工具）。

>[!Note]
>安装文件位于 [setup 分支](https://github.com/lab-iu6/TMS320C5515/tree/setup)

----------------
## 使用 Code Composer Studio 环境

程序的主要代码位于 `main.c` 文件中，其位置如下图所示：
<div align="center">
   <img src="https://github.com/user-attachments/assets/15b0764d-da84-42f7-96e2-32c48144857c" alt="main" width="800"/>
</div>

如果需要打开代码中引用的文件，可以使用 `Open Declaration` 功能。首先选中包含目标文件的代码行，然后使用该功能打开文件、函数描述或变量初始化（如下图所示）。
<div align="center">
   <img src="https://github.com/user-attachments/assets/6252d4a7-9151-4df1-9eae-ad750a48f64a" alt="open_decl" width="800"/>
</div>

每个项目包含以下几个部分：

- **Binaries 部分**：编译后生成的二进制可执行文件（`*.out`）会存放在此文件夹中。
- **Includes 部分**：列出了所有在编译选项中引用的目录，以及项目中提到的所有 `*.h` 和 `*.inc` 文件。
- **Debug 部分**：包含所有编译文件，包括 `*.map` 和 `*.obj` 文件。

源代码文件列表会显示在这些部分之后。如果源代码文件包含函数、引用其他文件或结构体，可以点击文件名前的箭头查看其包含的所有对象。双击对象名称可以跳转到代码中对应的位置。需要注意的是，`Binaries` 和 `Debug` 部分只有在项目首次成功编译后才会出现。

对于创建或导入的项目，可以执行以下操作：

- **编译项目**：点击工具栏上的"锤子"图标，进行项目构建并检查错误。
- **将代码加载到开发板**：点击工具栏上的 `Run -> Load`，选择要加载的项目（如下图所示）。
<div align="center">
   <img src="https://github.com/user-attachments/assets/fa61f1cd-076b-49ba-b2c9-d7b7e633be4b" alt="run" width="800"/>
</div>

- **调试模式运行程序**：点击工具栏上的"甲虫"图标，进入调试模式，可以监控变量值、设置断点、单步执行代码等（如下图所示）。
<div align="center">
   <img src="https://github.com/user-attachments/assets/54dd03c2-7d02-4fb4-a247-deb6623d7320" alt="debug" width="800"/>
</div>

要打开现有项目，请点击左上角的 `File -> Import`，选择项目类型为 `CCS Projects`（如下图所示），然后在文件系统中选择项目文件夹。
<div align="center">
   <img src="https://github.com/user-attachments/assets/0e78d304-6aff-42b7-8e16-609f2e954251" alt="import" width="800"/>
</div>

适合导入的项目会显示在 `Discovered projects` 列表中（如下图所示）。
<div align="center">
   <img src="https://github.com/user-attachments/assets/6d5fc1bf-6b7b-40d7-98b8-f397f7f363ae" alt="disc_proj" width="800"/>
</div>

----------------
## 使用测试项目
  
德州仪器随开发板提供了一个安装光盘，其中包含 IDE 和各种示例代码，但该光盘的目标平台是 Windows XP。所有必要的文档、示例和项目的原始文件夹位于 [setup 分支的对应文件夹](https://github.com/lab-iu6/TMS320C5515/tree/setup/usbstk5515_v1)。

这些项目与新版 CCS 不兼容，但以下内容可能有用：
- 源代码
- 文档

测试代码已移植到现代 CCS 中，位于 [test 文件夹](https://github.com/nargi3/TMS320C5515/tree/master/test)。与原始组织方式不同，所有示例代码都集中在一个 `main()` 函数中。可以通过组合不同的示例代码来运行，也可以通过注释掉相关代码行来禁用某些示例。

>[!Warning]
>这种实现方式是为了简化设备的初学者使用。
>由于微控制器资源非常有限，在这种实现方式下，内存容量成为瓶颈，因此在 `nor_writer` 示例中，将 `tx` 和 `rx` 的大小从 `4000 * 2` 字节减少到 `400 * 2` 字节。

----------------
## 配置自己的项目

要为 TMS320C5515 开发板创建项目，需要在 CCS IDE 中创建一个新项目，并设置以下配置：

<div align="center">
  <img src="https://github.com/user-attachments/assets/3c4af697-6460-4a1d-8bde-de845e32ea3a" alt="img" width="600"/>
</div>

在选择 Compiler Version 时，需要点击 More -> Add，然后选择安装 Code Generation Tool 的文件夹。

>[!Warning]
>`Compiler version` 必须手动指定，如果未安装 `Code Generation Tools`，将无法选择。
