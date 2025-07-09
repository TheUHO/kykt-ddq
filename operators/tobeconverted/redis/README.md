# ``op_redis``

这里应该写一组ddq的算子，包装redis的基本功能。

这些算子并不需要自己完成所有任务，实际上，大部分功能应当使用hiredis库来完成：
```
https://github.com/redis/hiredis
```
算子的代码只是把hiredis库的一些功能包装起来而已，可以参考其他算子的实现方式。


## 数据结构

ddq的算子是形如``task_ret op_redis_...(void **inputs, void **outputs, void **attribute);``的C语言函数，它的输入输出都是指针数组，数组长度不定。

注意到，在``common_objs.h``里定义了``obj_mem``和``obj_var``类型的结构体，指向它们的指针可以作为算子的输入输出。
其中``obj_mem``可以储存C风格的字符串或二进制数据（后面提到的字符串均指``obj_mem``），``obj_var``保存整数或实数。

在hiredis里定义了``redisContext``类型，它的指针也应当作为算子的输入输出。

除此之外的东西不会作为算子的输入或输出。


## 算子设计

### 建立链接的算子
```
task_ret op_redis_connect(void **inputs, void **outputs, void **attribute);
```
它接受三个输入，得到一个输出。
也就是说：

- inputs[0] 是个字符串，表示redis服务器的地址
- inputs[1] 是个整数，表示redis服务器的端口
- inputs[2] 是个字符串，表示redis服务器的密码
- outputs[0] 是个``redisContext``类型的指针，表示连接上服务器之后得到的上下文句柄。

这个算子包装的主要是``redisConnect()``函数，以及后续还要用``redisCommand()``函数验证密码。

如果发生任何错误，outputs[0]里返回NULL。


### 运行命令的算子
```
task_ret op_redis_command_set(void **inputs, void **outputs, void **attribute);
```
它接受不定数目的输入，得到0个输出。

这个算子负责包装``redisCommand()``函数。
因为这个函数有点类似printf/scanf的样子，是不定长的，因此算子的输入也是不定长的。
但有些输入输出是固定的：

- inputs[0] 是``redisContext``类型的指针，它将作为调用``redisCommand()``函数的第一个参数。
- inputs[1] 是个字符串，它将作为调用``redisCommand()``函数的第二个参数。

在inputs[1]里包含一个命令和若干参数，这些参数对应了后续的inputs[...]。根据命令的不同，参数的含义也不同，类型也可能不同。
参数的类型决定了对应位置的inputs[...]可能是``obj_mem``和``obj_var``类型的指针。

命令的类型限制为无输出结果的命令，比如SET、HSET、LPUSH等。如果不是这类命令或不支持的命令，应报错退出。
如果发生任何错误，也报错退出。

```
task_ret op_redis_command_getmem(void **inputs, void **outputs, void **attribute);
```
它接受不定数目的输入，得到1个输出。

这个算子的输入和前述相同，只是命令的类型限制为有输出结果的命令，比如GET、HGET、LPOP、LLEN等。如果不是这类命令或不支持的命令，应报错退出。
如果发生任何错误，包括返回的类型不对，报错退出。

它的输出是固定的：

- output[0] 是个``obj_mem``的指针，用于承接返回结果

```
task_ret op_redis_command_getvar(void **inputs, void **outputs, void **attribute);
```
它接受不定数目的输入，得到1个输出。

这个算子除了输出的类型有所不同，其他与``op_redis_command_getmem``相同：

- output[0] 是个``obj_var``的指针，用于承接返回结果


### 建构和析构函数

仿照``obj_mem_new``和``obj_mem_del``的写法，对``redisContext``对象做类似的事情。




