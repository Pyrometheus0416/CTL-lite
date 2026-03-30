#ifndef __CTL_H__
#define __CTL_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define FreeWarning fprintf(stderr,\
    "\033[32m"\
    "Warning: Pop all elements in container before free container! "\
    "Otherwhile, there is nothing will happend. \n"\
    "\033[0m");

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
 * foreach(vec_int, &v, iter){
 *      printf("%d\n", *iter.ref);
 * }
 * ```
 */
#define foreach(c, cp, iter) for(JOIN(c, it) iter = JOIN3(c,it,each)(cp); !iter.done; iter.step(&iter))

#define len(a) (sizeof(a) / sizeof(*(a))) // Carculate length of array

//-------------------------------------------------------------------

static inline size_t MAX_CTL(size_t a, size_t b) { return a>b ? a: b; }
static inline size_t MIN_CTL(size_t a, size_t b) { return a<b ? a: b; }

//-------------------------------------------------------------------

void ARRAY(int col, int arr[], size_t col_width){
    if(col==0){ puts("The Array is Empty!"); return; }
    char *table[3][3] = {
        "┌", "┬", "┐\n", // Head Row
        "│", "│", "│\n", // Number Row
        "└", "┴", "┘\n", // Tail Row
    };
    for(int i=0,q=0; i<3; ++i){
        for(int j=0; j<2*col+1; ++j){
            q = (j!=0) + 2*(j&1) + (j==2*col);
            if(q==3){ // Number Column
                if(i==1)
                    printf("%-*d", col_width, arr[j/2]);
                    // Use '*' as a placeholder of col_width
                else for(int k=0; k<col_width; ++k)
                    printf("─");
            }else printf(table[i][q]);
        }
    }
    // system("pause");
}

void TABLE(int row, int col, int arr[row][col], size_t col_width){    
    char *table[4][4]={
        "┌","┬","┐\n","─", // Head Row
        "├","┼","┤\n","─", // Middle separate Row
        "└","┴","┘\n","─", // Tail Row
        "│","│","│\n",""   // Number Row
    };

    for(int i=0,p=0,q=0; i<2*row+1; ++i){
        p = (i!=0) + 2*(i&1) + (i==2*row);
        for(int j=0; j<2*col+1; ++j){
            q = (j!=0) + 2*(j&1) + (j==2*col);
            if(q==3){ // Number Column
                if(p==3) // Number Row
                    printf("%-*d", col_width, arr[i/2][j/2]);
                    // Use '*' as a placeholder of col_width
                else for(int k=0; k<col_width; ++k)
                    printf(table[p][3]);
            }else printf(table[p][q]);
        }
    }
}

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