#ifndef __CTL_H__
#define __CTL_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define CAT(a, b) a##_##b
#define CAT3(a, b, c) a##_##b##_##c
#define JOIN(prefix, name) CAT(prefix, name)
#define JOIN3(prefix, name, suffix) CAT3(prefix, name, suffix)

#define SWAP(TYPE, a, b) { TYPE temp = *(a); *(a) = *(b); *(b) = temp; }

/**
 * @brief General Iterator Syntex
 * @param c Container Class
 * @param cp Container Instance Pointer
 * @param iter Iterator Instance
 * @return CTL-lite Forloop Head
 * 
 ---
 * ```c
 * foreach(array_int, &v, iter){
 *      printf("%d\n", *iter.ref);
 * }
 * ```
 */
#define foreach(c, cp, iter) for(JOIN(c, it) iter = JOIN3(c,it,each)(cp); !iter.done; iter.step(&iter))

#define len(a) (sizeof(a) / sizeof(*(a))) // Carculate length of array

//-------------------------------------------------------------------
static inline long MAX_CTL(long a, long b) { return a>b ? a: b; }
static inline long MIN_CTL(long a, long b) { return a<b ? a: b; }
//-------------------------------------------------------------------

/**
 * @brief Measure and print execution time of a function
 * @param f Function to time (returns void*, takes void*)
 * @param fmt Printf format string (default: colored "Program Run %5.3lf sec." if NULL)
 * @param number Multiplier for measured time (use for averaging multiple iterations)
 * @return Return value from f()
 * 
 * @note Uses `clock()` from <time.h> - measures CPU time, not wall-clock.
 * @note If `fmt == NULL`, defaults to: "\033[32m Program Run %5.3lf sec. \033[0m\n".
 * @note `number` scales the measured time: `(end-start) * number / CLOCKS_PER_SEC`.
 * 
 * Common Examples:
 * ```c
 * TIMEIT(my_func, NULL, 1);              // Default colored output
 * TIMEIT(my_func, "%.3f ms\n", 100);     // Average of 100 runs
 * ```
 */
void* TIMEIT(void* (*f)(void), char* fmt, size_t number){
    clock_t start_time=0, end_time=0;
    double unit = CLOCKS_PER_SEC;
    
    if(!fmt) fmt = "\033[32m Program Run %5.3lf sec. \033[0m\n";
    puts("=============== Start Timer ===============");
    start_time = clock();
    void* p = f();
    end_time = clock();
    puts("=============== Stop Timer ================");
    clock_t cost = (end_time - start_time) * number;
    printf(fmt, cost/unit);
    return p;
}

//-------------------------------------------------------------------
#endif