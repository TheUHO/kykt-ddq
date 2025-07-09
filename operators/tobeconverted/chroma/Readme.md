# DDQ-QDP

## 简介

DDQ-QDP是一些接口代码，用于把Chroma的一些功能包装起来，使它们可以被DDQ调度。


## 本阶段代码目标

实现用DDQ调度Chroma的计算夸克传播子和强子关联函数的完整功能，包括：

-  基本文件IO：组态、传播子等
-  规范场变换：涂摩
-  制造源：point source, shell source
	- 其中涂摩和平移功能是独立的算子
-  计算传播子：CG、BiCGstab
-  缩并汇(sink)
	- 可复用涂摩算子
-  计算强子2点关联函数


## 任务步骤

- 一(0-30%)、阅读Chroma、QDP++等源代码并完成**调研报告**，明确如下接口细节：
	- 本任务相关的所有数据对象运行时在内存中的储存格式，比如multi1d<Lattice...>等
		- 这些数据对象包括所有需要的算子的输入输出所涉及的内存对象，可以是QDP对象或自定义对象等
		- 应当以完整的文档形式，详细列举数据对象中的所有成员的内存布局
		- 不同数据对象中内存布局相同的公共部分(比如基类等)，可以独立成章
		- 有些数据对象的内存布局和环境参数等可能与Chroma、QDP++等软件的编译时参数有关，那么这些参数的所有选项均需要调研，包括：
			- QMP的--with-qmp-comms-type
			- QDP++和Chroma的--enable-parallel-arch
			- QDP++的Chroma的--enable-precision
			- QDP++的--enable-layout
	- 根据前述总体目标的要求，拆解需求，并完整列举所有算子
		- 算子可以在Chroma或QDP++源代码中对应一个或一组序贯调用的函数
		- 算子可以对应数据对象的产生/转换/销毁等过程
		- 算子可以是自定义功能，用于连接其他算子或数据对象
		- 算子可以是自定义功能，用于重写可能存在的难以包装的功能
		- 算子保持QDP对QMP及MPI的封装、保持Chroma和QDP++对openmp的封装
	- 目前初步统计的算子列表至少应包括：
		- 读入/写出组态文件
		- 读入源向量文件
		- 写出传播子文件
		- 导出关联函数/写入文件
		- 产生点源
		- 对源进行平移(displacement)
		- 对组态进行stout-link涂摩
		- 对源/汇进行规范不变涂摩
		- 用CG/BiCGstab算法计算传播子
		- 从传播子缩并到汇
		- 从汇计算介子、重子、流的关联函数，包括动量投影。基本上覆盖Chroma的"HADRON_SPECTRUM"提供的所有计算功能
		- 对以上算子涉及到的内存对象的初始化
- 二(30-80%)、逐一实现列表中的所有算子的**程序**：
	- 用C语言接口把原来的C++代码包装起来，使用void **指针数组传递输入输出的所有信息
		- 各种元信息，比如物理参数、算法参数、环境参数等，可以用struct包装为数据对象
		- 全局变量应包装为数据对象作为输入/输出参数
		- 保证代码无副作用
	- 写**文档**详述输入输出所涉及的数据对象
		- 保证算子之间的数据兼容
		- 尽量使数据结构具有通用性
		- 所有堆内存的申请和释放操作均需要注明
		- 所有副作用(如果实在无法避免的话)均需要注明
	- 算子的包装代码与QDP++的源代码分离存放，实际提交的代码中不包含USQCD-SciDac系列软件的任何部分
		- 编译脚本应该根据依赖关系分别编译不同软件，然后做动态链接，保证ABI兼容
- 三(80-100%)、测试验证，并完成**测试报告**：
	- 针对类似chroma/hadron/hadspec的功能编写一系列DDQ脚本和Chroma脚本，对所有算子组合进行完整覆盖测试，并与Chroma的对应算例的结果比对，要在浮点数允许精度内一致
	- 要至少支持两种运行环境：x86处理器的单进程串行、x86处理器集群的多节点MPI并行
		- MPI应由QDP++所使用的QMP支持，理论上本代码只需要传递适当参数即可
		- DDQ使用pthread启动算子，与算子内部的openmp、MPI等并行编程模型理论上可以兼容
	- 在每种运行环境下分别通过修改运行时的各种环境变量进行调优，并测量运行效率或运行时间，与Chroma进行比较



## 软件环境配置
chroma算子依赖USQCD Chroma及相关软件包，使用预配置脚本chroma_install.mk可安装chroma及相关软件包。
该脚本通过git自动下载源代码，并进行相应的编译和安装，运行之前建议先理解chroma_install.mk，以便出现问题时调试。
chroma_install.mk识别CHROMA_PATH和CHROMA_BUILD_TYPE两个环境变量，分别表示Chroma及相关软件包的安装软件和编译的类型，编译类型为cmake支持的Debug、Release等。

执行命令如
```bash
export CHROMA_PATH=you/install/path
export CHROMA_BUILD_TYPE=Release #或者Debug
cd CHROMA_INSTALL_DIR
make -f path/to/chroma_install.mk all
```

安装好Chroma及相关软件包之后，进入ddq/operators/chroma安装各个算子。
其中提供Makefile脚本，识别CHROMA_PATH、DDQ_PATH和XML2_INC环境变量，
自定义安装自需要修改这些环境变量值，不需要修改Makefile本身。

```bash
export CHROMA_PATH=you/install/path
export DDQ_PATH=${HOME}/usqcd/ddq.git
export XML2_INC=/usr/include/libxml2
make all
```

安装成功之后,运行测试命令
```bash
make test
```

对于chroma算子开发者，建议使用vscode IDE，将chroma、qdpxx、ddq等目录放在同一级目录，如
```
alwin@alwin-pc:~/simdev/usqcd$ tree -L 1
.
├── build
├── chroma
├── ddq.git
├── qdp-jit
├── qdpxx
├── qmp
└── quda
```
这样使用vscode可同时定位chroma代码和ddq代码，方便代码之间的切换。


使用时简便的方式是配置自己的env.sh
```bash
cd operators/chroma
cp etc/env_sample.sh env.sh
vi env.sh #并修改为自己的路径环境
source env.sh
```
之后再进行chroma_install.mk和Makefile的调用，会自动识别自定义环境变量。

## 目前进度

- 正确性测试
   - 算子组合进行完整覆盖测试：结果对比：invert、关联函数；DDQ调度带来不确定性。
   - 并行方式测试：串行、mpi并行、pthread+openmp并行，QUDA环境下测试是否有异常
   - 保证代码无副作用：线程安全（执行顺序无关），如有要注明
   - 注明算子内部是否有堆内存的申请和释放操作均需要注明（当前无）
- DDQ优势测试：
  - 调度并发的性能优势测试（当前运行有些慢）；DDQ+Chroma比Chroma更快
  - 算子功能组合优势，小算子和大算子
