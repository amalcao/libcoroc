# 关于CoroC编译器及运行时库的构建

## 具体步骤如下：

1. 从 git 仓库下载最新的 Clang-Coroc 代码: `git clone https://amalcao@bitbucket.org/amalcao/clang-coroc.git`

2. 从llvm.org下载[llvm3.5.0](http://llvm.org/releases/3.5.0/llvm-3.5.0.src.tar.xz)并解压（`tar Jxf llvm-3.5.0.src.tar.xz`），然后下载[compiler-rt-3.5.0](http://llvm.org/releases/3.5.0/compiler-rt-3.5.0.src.tar.xz)源代码，解压到llvm的projects子目录中，并重命名为`compiler-rt`。

3. 编译llvm3.5和clang-coroc-3.5源代码，具体方法是：

	- 将clang-coroc-3.5源码目录移至llvm源码目录的tools子目录下；
	- 在源码目录外单独创建一个用来进行编译工作的目录，运行`cmake`或`configure`进行配置（*可以省略一些无关项以加快编译流程*）；
	- 生成Makefile后，通过`make && make install` 进行编译、安装。
	- 注意：由于新版clang代码采用了一些C++0x/C++11标准的特性，因此要求C++编译器及标准C++库支持最新的C++语言标准，因此最好升级到gcc4.7（>= libstdc++-4.7）以上版本。

4. 使用CoroC的编译器命令行工具：

	- clang-coroc 编译安装成功后，将安装路径下的`bin`子目录加入系统的`PATH`环境变量中
	- 在终端运行`clang-co`命令进行CoroC代码的编译工作：`clang-co -rewrite-coroc input.co`
	- 编译成功后会生成一个后缀名为".cc"的C++文件，就是翻译后的文件；如果提示找不到`clang-co`，那么请检查安装是否正确以及PATH变量设置情况。	

5. 编译runtime及其示例程序
	
	- 将新编译好的clang二进制可执行文件安装路径添加到PATH环境变量中；
	- 直接进入runtime的源码路径，键入`make`进行编译，如果编译正确，则当前目录下会出现一个`bin`子目录，其中包含了测试程序；如果出错，检查一下Makefile的相关设置，如编译器版本等，修改后再进行编译。

6. 利用gdb进行程序调试

	- 按照普通方式启动gdb调试，如： `gdb ./bin/primes.run` ；
	- 运行 `r` 命令前，先加载 runtime 目录下的 `scripts/gdb_helper.py` 脚本，方法是在gdb的CLI交互界面下输入 `source where/is/gdb_helper.py`（*将路径替换为实际的路径*）；
	- 成功加载后，就可以运行针对CoroC运行时设计的新命令了，如`info coroutine` 等，具体参见 runtime 目录下的 readme.md 文件。
	

## 备注

鉴于CoroC开发尚处于初级阶段，测试并不完善，各种问题可能会比较多，请大家及时将BUG或问题反馈给我（amalcaowei@gmail.com），以便及早修正。

另外，本文档内容可能比较简略，也希望大家在实际操作过程中根据自己的经验，不断完善本文档，以便后面加入的同学可以节省一些时间，谢谢大家！