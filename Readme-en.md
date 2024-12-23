# How to Install CCS TI for old c55

1) Download and install the latest version from [TI website](https://www.ti.com/tool/CCSTUDIO) (or from the [attached file](https://github.com/lab-iu6/TMS320C5515/blob/setup/установка/ccs_setup_12.8.1.00005.exe))

   During installation, it's sufficient to select the C55x ultra-low-power DSP component, as shown in the image below.
<div align="center">
  <img src="https://github.com/user-attachments/assets/7468cebd-9f54-4aa0-885b-90bf8de96808" alt="setup_components" width="800"/>
</div>

3) Download and install [Code Generation Tools](https://github.com/lab-iu6/TMS320C5515/blob/setup/установка/ti_cgt_c5500_4.4.1_setup_win32.exe) (contains compiler, assembler, linker, etc.).

>[!Note]
>Installation files are located in the [setup](https://github.com/lab-iu6/TMS320C5515/tree/setup) branch

----------------
## Working with CodeComposerStudio Environment

The main program code is in the ``main.c`` file, located as shown in the image below.
<div align="center">
   <img src="https://github.com/user-attachments/assets/15b0764d-da84-42f7-96e2-32c48144857c" alt="main" width="800"/>
</div>

If you need to open a file that is included in the code, you can use the ``Open Declaration`` function by first highlighting the line with the desired file. This way you can open files, descriptions of highlighted functions, or initialization of selected variables (see image below).
<div align="center">
   <img src="https://github.com/user-attachments/assets/6252d4a7-9151-4df1-9eae-ad750a48f64a" alt="open_decl" width="800"/>
</div>

Each project has several sections:

``Binaries`` section — after compilation, the binary executable file ``*.out`` will be located in this folder.

``Includes`` section — lists all directories referenced in compilation options. The list is supplemented with all ``*.h`` and ``*.inc`` files mentioned in the project.

``Debug`` section — contains all compilation files, including ``*.map`` and ``*.obj``.

Then all source files are listed. If source code files contain functions, include other files or structures, you can click the arrow next to the file name to see a list of all objects contained in it. Double-clicking on an object name takes you to the location in the program code where the corresponding function, included file, or structure is located. Note that the ``Binaries`` and ``Debug`` sections only appear after the first successful project compilation.

Your created or imported project can be:

- compiled by clicking the ``hammer`` icon. During this operation, the project will be built and checked for errors;
- load code into the debug board (flash it). To do this, click ``Run -> Load`` on the top panel and select the project to flash (see image).
<div align="center">
   <img src="https://github.com/user-attachments/assets/fa61f1cd-076b-49ba-b2c9-d7b7e633be4b" alt="run" width="800"/>
</div>
- run the program on the microcontroller in debug mode to monitor variable values, use breakpoints, step through code, etc. To start debug mode, click the ``bug`` icon on the top panel (see image below).
<div align="center">
   <img src="https://github.com/user-attachments/assets/54dd03c2-7d02-4fb4-a247-deb6623d7320" alt="debug" width="800"/>
</div>

To open an existing project, click ``File -> Import`` in the upper left corner, select the project type ``CCS Projects`` (see image below) and select the project folder in your file system.
<div align="center">
   <img src="https://github.com/user-attachments/assets/0e78d304-6aff-42b7-8e16-609f2e954251" alt="import" width="800"/>
</div>

Projects suitable for import will appear in the ``Discovered projects`` section (see image below).
<div align="center">
   <img src="https://github.com/user-attachments/assets/6d5fc1bf-6b7b-40d7-98b8-f397f7f363ae" alt="disc_proj" width="800"/>
</div>

----------------
## Working with Test Project

Texas Instruments provides an installation disk with the board containing IDE and various examples, however, the target platform for this disk is Windows XP. The original folder with all necessary documentation, examples, and projects is located in the setup branch, in the [corresponding folder](https://github.com/lab-iu6/TMS320C5515/tree/setup/usbstk5515_v1).

Projects are not compatible with new CCS versions, but useful items might be:
- source code
- documentation

The test code has been transferred to modern CCS. It is located in the [test](https://github.com/nargi3/TMS320C5515/tree/master/test) folder. Unlike the original organization where each example was contained in a separate project, in this example all examples are fitted into one ``main()`` function. Calls to different examples can be combined, and you can remove the execution of any example from the program by simply commenting out the corresponding line.

>[!Warning]
>This solution was adopted to simplify familiarization with the device.
>Microcontroller resources are very limited, in this implementation RAM is the bottleneck, it simply cannot fit all global variables/arrays, so in the nor_writer example, the volumes of ``tx`` and ``rx`` are reduced from ``4000 * 2`` bytes to ``400 * 2``

----------------
## Setting Up Your Own Project

To create a project for the TMS320C5515 board, you need to create a project in CCS IDE, during project setup you need to specify the following settings:

<div align="center">
  <img src="https://github.com/user-attachments/assets/3c4af697-6460-4a1d-8bde-de845e32ea3a" alt="img" width="600"/>
</div>

When selecting Compiler Version, you need to click More -> Add -> and select the folder where Code Generation Tool was installed.
> [!Warning]
> ``Compiler version`` must be specified manually, accordingly, if ``Code Generation Tools`` was not installed, you won't be able to select it.
