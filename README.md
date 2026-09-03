# C Lightweight Template Library (CTL-lite)

[中文](README.md)

CTL-lite is a header-only, type-safe STL-like library for ISO C99/C11 that compiles fast.

## Goals

CTL boosts developer productivity by implementing the following STL containers in ISO C99/C11:

| Header     | STL counterpart        | Implementation                              |
| :--------: | :--------------------- | :------------------------------------------ |
| `array.h`  | `std::vector`          | Contiguous memory, dynamic growth via `realloc` |
| `deque.h`  | `std::deque`           | Paged (chunked) storage                     |
| `heap.h`   | `std::priority_queue`  | Max-heap (complete binary tree on an array) |
| `set.h`    | `std::unordered_set`   | Hash table with singly-linked buckets (open hashing) |
| `str.h`    | `std::string`          | Concrete string type (not a template), built-in KMP search |

> Container instances are named by *type prefix + element type*, e.g. `arr_int`, `deq_float`, `set_int`.
> `str` is a fixed type and does not require `#define T`.

Design highlights:

- **Fast compilation** — everything is headers plus macro templates; no C++ template metaprogramming overhead. Only the standard C headers (`stdlib`/`stdio`/`stdint`/`time`) are used.
- **Type safety** — token pasting generates a distinct strongly-typed container for each element type, so type errors surface at compile time.
- **Opt-in traits** — `ORDERED` / `HASH` / `FMT` are enabled only when you define them; no code is generated for unused traits.

## Usage

`#define T` as a built-in type or a typedef'd type.  
You may also specify *traits* for the type to add extra functionality.

If you need different ORDERED or HASH variants of the same type, alias it with `typedef` under distinct names.

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

## Data Traits

The supported traits are `ORDERED`, `HASH`, and `FMT`. They apply to the element type `T` and are **not** auto-`undef`'d:

- If a trait is not defined, the corresponding behavior is not added to `T`.
- If defined as `0`, the data is a built-in type and the matching functions are generated automatically using the predefined methods in `typing.h`.
- If defined as `1`, you must **provide the implementation function yourself**.
- Any other non-zero integer behaves like `1`, but is reserved for future extension.

Providing an implementation function usually unlocks useful utility functions generated from the trait:

| Trait   | Impl. function | Meaning        | Utility functions |
| :-----: | :------------: | :------------- | :---------------- |
| ORDERED | T_compare      | Well-ordering  | MAX_T, MIN_T      |
| HASH    | T_hash         | Hashability    | T_equal           |
| FMT     | T_print        | Formatted I/O  | ARRAY_T, TABLE_T  |

Implementation function signatures:

- `short JOIN(T, compare)(T* a, T* b);`
- `uint64_t JOIN(T, hash)(T* a);`
- `void JOIN(T, print)(T a, size_t col_width);`
  - In printf format strings, `*` can be used as a column-width placeholder: `%-*d`.

## Memory Ownership

The container owns the lifetime of its elements: on insertion it keeps a private copy via `elem_copy`; on removal or teardown it destroys elements via `elem_free`.

### Built-in types (default)

For C built-ins such as `int`, `double`, or `char`, no extra declarations are needed. The container uses *implicit copy* (a plain value copy of `*data`) and leaves `elem_free` as `NULL` — the elements require no cleanup.

### Explicit lifetime: define `EXPLICIT`

When the element type `T` requires explicit copy and release (e.g. structs, pointers, resource handles), define `EXPLICIT` and declare two functions **before including each container header**:

- `T JOIN(T, copy)(T*)` — the copy constructor (deep copy recommended); returns an independent copy
- `T JOIN(T, free)(T*)`  — the destructor; frees resources held inside the element

```c
typedef struct type { ... } type;
type type_copy(type*);   // deep copy
void  type_free(type*);  // free resources
#define T type
#define EXPLICIT
#include "ctl-lite/array.h"
```

At `init`, the container binds `elem_copy` / `elem_free` to `type_copy` / `type_free`.

### Conventions

- **Storing**: operations like `push` / `insert` / `add` / `set` store the result of `elem_copy(&v)`, **not the object you passed in**; you keep ownership of the original `v` and may continue to use or free it yourself.
- **Retrieving**: `pop(&c, cache)` (and `popr` on deques) copies the removed element into `cache` via `elem_copy`, then frees the internal copy; passing `NULL` frees without retrieving. (`set`'s `remove` deletes by value and frees the node directly.)
- **Teardown**: `free(&c)` calls `elem_free` on every element in the container; do not access those elements afterwards.
- **Copying**: `*_copy(&c)` builds a new container, deep-copying every element through `elem_copy`.
- **Deep copy**: a deep-copying `type_copy` is strongly recommended. If you implement a shallow copy, you must keep things consistent yourself — e.g. always store deep-copied results, or always take retrieved values by pointer, to avoid double frees / dangling pointers.
- **Re-declaring**: a container header `#undef`s `EXPLICIT` after using it, so when instantiating the same type in multiple containers you must **re-`#define EXPLICIT` before each container include**.

## Implementation Details

Every container is a *macro template*: you `#define T <element type>` before including the header, and the header uses token-pasting macros such as `JOIN` to generate a family of strongly-typed functions and structs prefixed with the type name. Including without `#define T` raises a compile-time `#error`.

### ctl.h — macro infrastructure

- `CAT` / `JOIN` — token pasting; concatenates a prefix and a name into an identifier (`arr_int`, `arr_int_push`, ...).
- `foreach` — the generic iterator loop macro; expands to a `for` header.
- `SWAP` — swaps two variables of the same type.
- `len` — length of a static array.
- `TIMEIT` — times a function call with `clock()` and prints the result; supports a custom format and averaging.

### array.h — dynamic array (≈ std::vector)

- Backed by a contiguous buffer `T* value`; used range `[0, size)`, reserved `capacity` (initially 8), with `capacity >= size` always.
- Growth: `reserve` adjusts capacity smartly around `realloc` — requests below `size+1` are bumped up to `size+1`; requests above the current capacity but within `2*capacity` are doubled. `push` triggers `reserve(2*capacity)` once `size+1 >= capacity`, giving amortized O(1) push.
- Random access `at(i)` is simply `value + i`, O(1); `head`/`tail`/`begin`/`end` return pointers to the corresponding positions.
- Insertion/removal: `insert` shifts elements backwards from the tail, `erase` shifts them forwards — O(n) worst case; `set` frees the old element before writing the new copy.
- Iterator: besides full traversal via `each`, `range(begin, end, stride)` iterates a sub-range with an arbitrary stride.
- ORDERED trait: `sort` (quicksort over `[l, r]` using `compare`) and `bSearch` (binary search returning the match index / insertion point).
- HASH trait: `find` (index of the n-th element equal to `key`) and `equal` (element-wise comparison of two containers).
- Others: `filter` (in-place filtering — moves matches forward, then pops the tail), `reverse` (range reversal), `extend` (batch append).

### deque.h — double-ended queue (≈ std::deque)

- Paged storage: `pages` is an array of page pointers; each page is a `T[DEQ_PAGE_SIZE]` (default 4, overridable via `DEQ_PAGE_SIZE`).
- Logical-to-physical mapping: index `i` is offset by the head offset `a`, then split into `page = (a+i)/DEQ_PAGE_SIZE` and `offset = (a+i)%DEQ_PAGE_SIZE`.
- `prologue`/`epilogue` track the first/last *used* page; `a`/`b` track the offset of the head element within the first used page and the position one past the tail element within the last used page.

```
pages:   [     |  P0  |  P1  |  P2  |     ]
              ↑prologue        ↑epilogue
   ← front         used range         back →
```

- Front/back operations only adjust `a`/`b`, allocating or freeing pages when a page fills or empties — amortized O(1). Random access is O(1), but elements are spread across independently `malloc`'d pages, so memory is **not** contiguous.
- `reserve` re-centers the used-page range inside the page array when growing or shrinking, leaving room for growth on both ends.
- Initial capacity is 2 pages, with `prologue` starting at 1 to reserve a page in front.

### heap.h — max-heap priority queue (≈ std::priority_queue)

- A complete binary tree over the array `T* value`: the parent of node `n` is `(n-1)>>1`; children are `2n+1` and `2n+2`.
- Requires the ORDERED trait (a missing trait triggers `#error`); `compare` defines the priority order.
- `init(compare, recursion)`: when `recursion` is true, the recursive sift-up/down (`up`/`down`) are bound; otherwise the iterative versions (`up_`/`down_`) are used.
- `push` appends then sifts up — O(log n); `pop` swaps the root with the last element then sifts down — O(log n); `top` is the maximum element.
- The backing array grows by `+5` when full; initial capacity is 5.

### set.h — unordered set (≈ std::unordered_set)

- Open hashing (chaining): bucket array `B** buckets`; each bucket is a singly-linked list of `B{key, next}` nodes.
- Bucket index = `splitmix32(hash(key)) % bucket_count`; `splitmix32` is a mixing function that improves hash distribution.
- The bucket count is always a prime: `reserve` binary-searches a static table of 223 primes (`closest_prime`) for the nearest prime, since prime moduli reduce collisions.
- Auto-growth: when the load factor (`size / bucket_count`) exceeds 0.75, `reserve(bucket_count*2)` rehashes. Rehashing first *squeezes* every chain into a single list (`squeeze`), then `realloc`s the bucket array, zeroes it, and re-inserts each node by its new bucket index.
- Deduplication: `add` walks the bucket chain with `equal` and ignores duplicates.
- Set algebra: `subset` tests inclusion; `union` / `intersection` / `difference` return a new set (deep-copying elements) in O(n).
- The iterator advances bucket by bucket (in order) and walks each chain; the end sentinel is `(size_t)(-1)`, i.e. `SIZE_MAX`.
- Initial bucket count is 3.

### str.h — string (non-template, fixed type)

- `str` is a concrete type and needs no `#define T`. Internally it holds `char* c_str` plus `len` and `capacity`, maintaining the invariants `capacity > len` and `c_str[len] == '\0'`.
- Growth mirrors `array.h` (doubling, preserving the used range).
- Range operations are described with the half-open interval `slice{l, r}` (`set` fills a range, `reverse` reverses a range, `replace` substitutes within a range, ...).
- Other utilities: `cat` (append), `upper`/`lower` (case conversion), `stride` (strips leading/trailing whitespace).
- KMP matching: `kmp_next` builds the failure table (using an offset trick, optimized into a nextval array) and `str_kmp` returns the first matching interval `[l, r)`, or `{0, 0}` if none is found.

### typing.h — type traits and method generation

- Generates methods for `T` from the `ORDERED` / `HASH` / `FMT` traits; traits are **not** auto-`undef`'d, so they affect every subsequent include.
- `ORDERED = 0`: auto-generates `T_compare` (returns the sign of the difference) plus `T_MAX` / `T_MIN`; `ORDERED = 1`: you provide `T_compare` and `T_MAX` / `T_MIN` are built on it.
- `HASH = 0`: auto-generates `T_hash` (a cast to `uint64_t`) and `T_equal` (`==`); `HASH = 1`: you provide `T_hash` and `T_equal` reduces to comparing two hash values.
- `FMT = 0`: auto-generates `T_print` via `_Generic` dispatch over built-in types (supports the `%-*` column-width placeholder); `FMT = 1`: you provide `T_print`.
- Printing utilities derived from FMT: `T_ARRAY` (1-D table) and `T_TABLE` (2-D grid).
