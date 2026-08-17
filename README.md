# C 轻量模板库 (CTL-lite)

CTL-lite 是一个编译速度快、类型安全、仅头文件的类模板库，适用于 ISO C99/C11。

## 目标

CTL 旨在通过在 ISO C99/C11 中实现以下 STL 容器来提高开发者的生产力：

```
deq.h = std::deque，使用分页的 realloc 实现
heap.h = std::priority_queue，大根堆
str.h = std::string，基于 array.h 实现
set.h = std::unordered_set，哈希前向链表
array.h = std::vector，使用 realloc 实现
```

## 使用方法

使用 `#define` 定义一个 `T` 为内置类型或 typedef 类型 `T`。  
同时，可以为这些类型指定“特性”，来为 `T` 添加特殊功能。  

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
    array_int a = array_int_init(compare);
    array_int_push(&a, 9);
    array_int_push(&a, 1);
    array_int_push(&a, 8);
    array_int_push(&a, 3);
    array_int_push(&a, 4);
    array_int_sort(&a, compare);
    foreach(vec_int, &a, it)
        printf("%d\n", *it.ref);
    vec_int_free(&a);
}
```

## 数据特性

目前支持的特性有 ORDERED、HASH、FMT。它们作用于数据类型 T 上，不会自动 undef，
- 如果不定义，表明不对数据 T 添加对应特性
- 如果定义为0，表明数据是内置的类型，并且使用 typing.h 中预定义的方法自动添加对应的函数
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

定义 `EXPLICIT` 表示类型 `T` 需显式复制和释放，  
并且在包含容器头文件之前，必须声明相当于 C++ 析构函数和拷贝构造函数的函数。

拷贝构造函数推荐实现深拷贝。如果实现的是浅拷贝，使用者需自行规划存入的行为。  
例如：总是存入深拷贝的结果或者总是在取出时使用指针接收。

```C
typedef struct { ... } type;
void type_free(type*); // 声明析构函数
type type_copy(type*); // 声明拷贝函数
#define T type
#include <array.h>
```

## 实现细节

双端队列：基于分页的动态内存分配

```
            prologue                         epilogue
└──────────┴───═══════╧══════════╧══════════╧═════─────┴──────────┘
               ↑a ------- [a, b) is used --------↑b   ↑b(maybe)
```

```h
typedef struct A{
    void (*elem_free)(T *);
    T (*elem_copy)(T *);
    B **pages; // pages[i] --> page; *page --> B; B[i] --> T

    size_t prologue; // Page offset  of first used page
    size_t epilogue; // Page offset of last used page
    size_t a; // Offset of head element in prologue page
    size_t b; // Offset past tail element in epilogue page (maybe)

    size_t capacity; // Size of all pages
    size_t size; // Number of All elements
} A;
```
