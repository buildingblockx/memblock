# memblock allocator

## 概述

在page allocator（example: buddy allocator）正式初始化之前，内核需要申请/释放内存，而且page allocator本身也需要申请/释放内存，所以需要一个简单易用的内存分配器将所有内存进行管理。于是，memblock allocator就诞生了。

## 原理

```
                    regions
                ┌─►┌───────┐
     memblock   │  │region │
    ┌────────┐  │  ├───────┤
    │        │  │  │region │
    │ memory ├──┘  ├───────┤
    │        │     │region │
    ├────────┤     └───────┘
    │        │      regions
    │reserved├────►┌───────┐
    │        │     │region │
    └────────┘     ├───────┤
                   │region │
                   ├───────┤
                   │region │
                   └───────┘
```

如上图所示，`memblock allocator`定义一个全局静态变量`memblock`，其中有两个重要的成员，分别是`memory`与`reserved`, `memory`成员指向可用内存区域，`reserved`成员指向预留内存区域。

每一个`memory`/`reserved`类型的`region`代表一个`可用`/`预留`内存区域，来自于全局静态数组`regions`（`memory_regions[]`/`reserved_regions[]`），默认都是128个`region`。如果系统中的`可用`/`预留`内存区域很多，128个不够，`memblock allocator`会动态扩展此数组。

在早期系统初始化过程中，需要将所有可用内存区域一一加入到`memory`类型的`region`中，进行统一管理。如果相邻两个可用内存区域是连续的，需要合并这两个可用内存区域为一个`memory`类型的`region`。

后面有可用内存区域A不想被`memblock allocator`进行统一管理，如果要删除的可用内存块区域A是可用内存区域B中的一小块，需要将可用内存区域B进行拆分成2~3个新`memory`类型的`region`，只删除可用内存块区域A对应的`memory`类型的`region`; 否则，直接删除可用内存区域A对应的`memory`类型的`region`即可。

当向`memblock allocator`申请内存时，存在`memory`类型的`region`并且不存在`reserved`类型的`region`中，查找合适大小的内存块，找到后，返回内存块的首地址，同时将此内存块区域加入`reserved`类型的`region`，代表此内存块区域已经被别人申请走

当释放内存到`memblock allocator`时，直接删除内存块区域对应的`reserved`类型的`region`即可

## 如何使用？

使用`memblock allocator`时，首先需要在系统启动前期中，调用`memblock_init()`，并且在此函数里面调用`memblock_add()`填充所有可用内存到`memblock allocator`，进行统一管理。这也是唯一需要用户手动写的源码，完成后，就可以使用`memblock_alloc()`/`memblock_free()`进行申请/释放内存。

使用时记得加上头文件：`#include <memory/allocator/memblock.h>`