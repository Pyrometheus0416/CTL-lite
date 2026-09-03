# C 轻量模板库 (CTL-lite)

[English](README.en.md)

CTL-lite 是一个编译速度快、类型安全、仅头文件的类模板库，适用于 ISO C99/C11。

## 目标

CTL 旨在通过在 ISO C99/C11 中实现以下 STL 容器来提高开发者的生产力：

| 头文件      | 对应 STL              | 实现方式                       |
| :---------: | :-------------------- | :----------------------------- |
| `array.h`   | `std::vector`         | 连续内存，`realloc` 动态扩容   |
| `deque.h`   | `std::deque`          | 分页（chunked）存储            |
| `heap.h`    | `std::priority_queue` | 大根堆（数组实现的完全二叉树） |
| `set.h`     | `std::unordered_set`  | 哈希 + 前向链表（开散列）      |
| `str.h`     | `std::string`         | 固定字符串类型（非模板），内置 KMP 匹配 |

> 容器实例由「类型前缀 + 元素类型」命名，例如 `arr_int`、`deq_float`、`set_int`；
> `str` 是固定类型，不需要 `#define T`。

设计要点：

- **编译快速**：全部是头文件 + 宏模板，不依赖 C++ 模板元编程，仅使用标准 C 头文件（`stdlib`/`stdio`/`stdint`/`time`）。
- **类型安全**：通过宏拼接为每个元素类型生成独立的强类型容器，类型错误在编译期即可发现。
- **特性可选**：`ORDERED` / `HASH` / `FMT` 特性按需开启，未使用时不会生成对应代码。

## 使用方法

使用 `#define` 定义一个 `T` 为内置类型或 typedef 类型 `T`。  
同时，可以为这些类型指定"特性"，来为 `T` 添加特殊功能。  

如果需要对同一个类型声明不同的 ORDERED 或 HASH，应当使用 `typedef` 处理为不同名称的类型。

```C
#include <stdio.h>

#define T int
#define ORDERED 0
#define HASH 0
#include "ctl-lite/typing.h" // init type method of T

#include "ctl-lite/array.h"
#include "ctl-lite/deque.h"
#undef T
#undef ORDERED
#undef HASH

int main(void)
{
    arr_int a = arr_int_init();
    arr_int_push(&a, 9);
    arr_int_push(&a, 1);
    arr_int_push(&a, 8);
    arr_int_push(&a, 3);
    arr_int_push(&a, 4);
    arr_int_sort(&a, 0, a.size - 1); // sort the range [l, r]
    foreach(arr_int, &a, it)
        printf("%d\n", *it.ref);
    arr_int_free(&a);
}
```

## 数据特性

目前支持的特性有 ORDERED、HASH、FMT。它们作用于数据类型 T 上，不会自动 undef，
- 如果不定义，表明不对数据 T 添加对应特性
- 如果定义为0，表明数据是的内置类型，并且使用 typing.h 中预定义的方法自动添加对应的函数
- 如果定义为1，表明需要使用者**自行添加**对应特性的实现函数
- 其它整数与1相同，但未来可由开发者自行扩展。

在添加特性实现函数后，往往会根据特性生成一些实用的工具函数。

|  特性   | 实现函数  |     说明     |    工具函数     |
| :-----: | :-------: | :----------: | :-------------: |
| ORDERED | T_compare |    良序性    |   MAX_T,MIN_T   |
|  HASH   |  T_hash   |   可哈希性   |     T_equal     |
|   FMT   |  T_print  | 可格式化打印 | ARRAY_T,TABLE_T |

实现函数的签名：
- `short JOIN(T, compare)(T* a, T* b)};`
- `uint64_t JOIN(T, hash)(T* a);`
- `void JOIN(T, print)(T a, size_t col_width);`，
  - 在 printf 的格式控制字符中，可以使用 `*` 作为列宽的占位符：`%-*d`。

## 内存所有权

容器负责其内部元素的生命周期：插入时通过 `elem_copy` 保存一份"自有副本"，移除或释放时通过 `elem_free` 销毁。

### 内置类型（默认）

对 `int`、`double`、`char` 等 C 内置类型，无需额外声明。容器默认采用"隐式拷贝"（直接按值复制 `*data`），并将 `elem_free` 置空——元素不需要释放。

### 显式生命周期：定义 `EXPLICIT`

当元素类型 `T` 需要显式复制与释放（如结构体、指针、资源句柄）时，定义 `EXPLICIT`，并**在包含每个容器头文件之前**声明两个函数：

- `T JOIN(T, copy)(T*)` —— 拷贝构造函数（推荐深拷贝），返回一个独立副本
- `T JOIN(T, free)(T*)`  —— 析构函数，释放元素内部持有的资源

```c
typedef struct type { ... } type;
type type_copy(type*);   // 深拷贝
void  type_free(type*);  // 释放资源
#define T type
#define EXPLICIT
#include "ctl-lite/array.h"
```

容器在 `init` 时会把 `elem_copy` / `elem_free` 分别绑定到 `type_copy` / `type_free`。

### 使用规范

- **存入**：`push` / `insert` / `add` / `set` 等操作存入的是 `elem_copy(&v)` 的结果，**不会保存调用方传入对象本身**；调用方对原对象 `v` 仍保有所有权，可继续使用或自行释放。
- **取出**：`pop(&c, cache)`（以及 deque 的 `popr`）会先把被移除的元素 `elem_copy` 到 `cache` 指向的位置，再释放容器内部副本；`cache` 传 `NULL` 则只释放、不取出。（`set` 的 `remove` 按值删除，直接释放被删元素。）
- **释放**：`free(&c)` 会对容器内所有元素逐一调用 `elem_free`，释放后不得再访问这些元素。
- **拷贝**：`*_copy(&c)` 生成新容器，逐元素调用 `elem_copy` 做深拷贝。
- **深拷贝**：强烈建议 `type_copy` 实现深拷贝。若实现浅拷贝，使用者需自行保证一致性，例如：总是存入深拷贝的结果，或总是用指针接收取出的值，避免重复释放 / 悬挂指针。
- **重复声明**：容器头文件使用 `EXPLICIT` 后会将其 `#undef`，因此对多个容器实例化同一类型时，**每次包含容器头文件前都需重新 `#define EXPLICIT`**。

## 实现细节

所有容器都是"宏模板"：包含头文件前先 `#define T <元素类型>`，头文件通过 `JOIN` 等宏做记号粘合，生成一组以类型名为前缀的强类型函数与结构体；未定义 `T` 时会触发 `#error`。

### ctl.h —— 宏基础设施

- `CAT` / `JOIN`：记号粘合，把前缀与名字拼成标识符（`arr_int`、`arr_int_push` 等）。
- `foreach`：迭代器循环宏，等价于一段 `for` 头。
- `SWAP`：交换两个同类型变量。
- `len`：求静态数组长度。
- `TIMEIT`：以 `clock()` 测量某个函数调用的耗时并打印，支持自定义格式与求平均次数。

### array.h —— 动态数组（≈ std::vector）

- 底层为连续缓冲区 `T* value`，使用区间 `[0, size)`，预分配 `capacity`（初始 8），恒有 `capacity >= size`。
- 扩容：`reserve` 基于 `realloc` 智能调整——请求容量不足 `size+1` 时保底到 `size+1`；请求超过当前容量且在 `2*capacity` 以内时直接翻倍。`push` 在 `size+1 >= capacity` 时触发 `reserve(2*capacity)`，均摊 O(1)。
- 随机访问 `at(i)` 即 `value + i`，O(1)；`head`/`tail`/`begin`/`end` 返回对应位置的指针。
- 插入/删除：`insert` 从尾部向前搬移元素，`erase` 从前往后搬移，最坏 O(n)；`set` 先释放旧元素再写入新副本。
- 迭代器：除全量遍历 `each` 外，支持 `range(begin, end, stride)`，可按任意步长访问子区间。
- ORDERED 特性：`sort`（对 `[l, r]` 做快速排序，借助 `compare`）、`bSearch`（二分查找，返回命中位置或插入点）。
- HASH 特性：`find`（查找第 n 个与 key 相等的元素下标）、`equal`（逐元素比较两容器是否相等）。
- 其它：`filter`（原址过滤：命中的元素前移，随后 pop 尾部）、`reverse`（区间逆序）、`extend`（批量追加）。

### deque.h —— 双端队列（≈ std::deque）

- 采用分页存储：`pages` 是指向页的指针数组，每页是一块 `T[DEQ_PAGE_SIZE]`（默认 4，可用 `DEQ_PAGE_SIZE` 宏覆盖）。
- 逻辑下标到物理位置的映射：`i` 先加上队头偏移 `a`，再拆成 `页号 = (a+i)/DEQ_PAGE_SIZE`、`页内偏移 = (a+i)%DEQ_PAGE_SIZE`。
- `prologue`/`epilogue` 记录首/末"已使用页"的页号，`a`/`b` 记录队首元素在首使用页内的偏移、以及队尾元素在末使用页内的后一位置。

```
            pages:   [     |  P0  |  P1  |  P2  |     ]
                          ↑prologue        ↑epilogue
           ← 队头方向         使用区间         队尾方向 →
```

- 两端操作只调整 `a`/`b`，页满/页空时增删页，均摊 O(1)；随机访问 O(1)，但元素分散在多块独立 `malloc` 的页中，不保证内存连续。
- `reserve` 扩容/缩容时会平移"使用页区间"，使其重新居中于页数组中部，为两端留出生长空间。
- 初始容量 2 页，`prologue` 从 1 起步，为队头预留一页。

### heap.h —— 大根堆优先队列（≈ std::priority_queue）

- 基于数组 `T* value` 的完全二叉树：节点 `n` 的父节点为 `(n-1)>>1`，左右孩子为 `2n+1`、`2n+2`。
- 依赖 ORDERED 特性（未定义直接 `#error`），以 `compare` 定义优先级。
- `init(compare, recursion)`：`recursion` 为真时绑定递归版堆化 `up`/`down`，为假时绑定迭代版 `up_`/`down_`。
- `push`：追加到末尾后上浮（sift up），O(log n)；`pop`：堆顶与堆底交换后下沉（sift down），O(log n)；`top` 即堆顶最大元素。
- 容量满时按 `+5` 扩容，初始容量 5。

### set.h —— 无序集合（≈ std::unordered_set）

- 开散列（链地址法）：桶数组 `B** buckets`，每个桶是一条单向链表，节点 `B{key, next}`。
- 桶下标 = `splitmix32(hash(key)) % bucket_count`；`splitmix32` 为辅助混合函数，用于改善散列分布。
- 桶数量恒为素数：`reserve` 通过二分查找 223 个素数组成的静态表（`closest_prime`）取最近的素数，素数取模利于减少冲突。
- 自动扩容：负载因子（`size / bucket_count`）> 0.75 时 `reserve(bucket_count*2)` 重散列。重散列先把所有链"挤"成一条链表（`squeeze`），再 `realloc` 桶数组并清零，最后逐节点按新桶号插回。
- 去重：`add` 沿桶链用 `equal` 查重，重复则忽略。
- 集合运算：`subset` 判子集；`union` / `intersection` / `difference` 返回新集合（对元素深拷贝），复杂度 O(n)。
- 迭代器按桶号从小到大推进，桶内沿链表遍历；末尾哨兵为 `(size_t)(-1)`（即 `SIZE_MAX`）。
- 初始 3 个桶。

### str.h —— 字符串（非模板，固定类型）

- `str` 是具体类型，无需 `#define T`。内部为 `char* c_str` + `len` + `capacity`，并维护不变量 `capacity > len`、`c_str[len] == '\0'`。
- 扩容策略与 array 一致（翻倍、保留使用区间）。
- 用左闭右开区间 `slice{l, r}` 描述范围操作（`set` 区间填充、`reverse` 区间逆序、`replace` 区间替换等）。
- 其它：`cat` 追加、`upper`/`lower` 大小写转换、`stride` 去除两端空白字符。
- KMP 匹配：`kmp_next` 构造失配跳转表（采用下标偏移技巧，并优化为 nextval 数组），`str_kmp` 返回首个匹配区间 `[l, r)`，未命中返回 `{0, 0}`。

### typing.h —— 类型特性与方法生成

- 按 `ORDERED` / `HASH` / `FMT` 三个特性为 `T` 生成方法；特性不会自动 `undef`，影响其后的所有包含。
- `ORDERED = 0`：自动生成 `T_compare`（返回符号差）与 `T_MAX` / `T_MIN`；`ORDERED = 1`：由使用者提供 `T_compare`，`T_MAX` / `T_MIN` 基于它实现。
- `HASH = 0`：自动生成 `T_hash`（强转为 `uint64_t`）与 `T_equal`（`==`）；`HASH = 1`：使用者提供 `T_hash`，`T_equal` 退化为比较两个哈希值。
- `FMT = 0`：基于 `_Generic` 按内置类型自动生成 `T_print`（支持 `%-*` 列宽占位符）；`FMT = 1`：使用者提供 `T_print`。
- 由 FMT 派生的打印工具：`T_ARRAY`（一维表格）、`T_TABLE`（二维表格）。
