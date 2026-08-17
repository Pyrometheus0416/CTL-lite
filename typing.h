/* Typing */
#include "ctl.h"

#ifndef T
#error "Template type T undefined for <typing.h>"
#endif

#ifdef ORDERED
#if ORDERED // `#define ORDERED true`
static inline T JOIN(MAX, T)(T a, T b) { return (JOIN(T, compare)(a,b)>0)? a: b; }
static inline T JOIN(MIN, T)(T a, T b) { return (JOIN(T, compare)(a,b)<0)? a: b; }
#else // `#define ORDERED false`
static short JOIN(T, compare)(T* a, T* b) { return (*a>*b)-(*a<*b); }
static inline T JOIN(MAX, T)(T a, T b) { return a>b ? a: b; }  // Ordinary compare
static inline T JOIN(MIN, T)(T a, T b) { return a<b ? a: b; }  // Ordinary compare
#endif
#endif

#ifdef HASH
#if HASH // `#define HASH true`
static bool JOIN(T, equal)(T* a, T* b) { return JOIN(T, hash)(a) == JOIN(T, hash)(b); }
#else
static uint64_t JOIN(T, hash)(T* a) { return (uint64_t)(*a); }
static bool JOIN(T, equal)(T* a, T* b) { return *a == *b; }
#endif
#endif

#ifdef FMT
#if FMT
;
#else
static void JOIN(T, print)(T a, size_t col_width){
    char* ctrl_sign = _Generic(a,
        // Use '*' as a placeholder of col_width
        char: "%-*c",
        bool: "%-*d",

        short: "%-*hd",
        unsigned short: "%-*hu",
        int: "%-*d",
        unsigned int: "%-*u",
        long: "%-*ld",
        unsigned long: "%-*lu",
        long long: "%-*lld",
        unsigned long long: "%-*llu",

        float: "%-*.3f",
        double: "%-*.3lf",
        long double: "%-*.3Lf",

        default: "%-*p"
    );
    printf(ctrl_sign, col_width, a);
}

#endif

static void JOIN(ARRAY, T)(size_t len, T arr[len], size_t col_width){
    if(len==0){ puts("∅"); return; }

    char *table[3][3] = {
        "┌", "┬", "┐\n", // Head Row
        "│", "│", "│\n", // Number Row
        "└", "┴", "┘\n", // Tail Row
    };
    for(int i=0,q=0; i<3; ++i){
        for(int j=0; j<2*len+1; ++j){
            q = (j!=0) + 2*(j&1) + (j==2*len);
            if(q==3){ // Number Column
                if(i==1)
                    JOIN(T, print)(arr[j/2], col_width);
                else for(int k=0; k<col_width; ++k)
                    printf("─");
            }else printf(table[i][q]);
        }
    }
    // system("pause");
}

void JOIN(TABLE, T)(size_t row, size_t col, T arr[row][col], size_t col_width){    
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
                    JOIN(T, print)(arr[i/2][j/2], col_width);
                else for(int k=0; k<col_width; ++k)
                    printf(table[p][3]);
            }else printf(table[p][q]);
        }
    }
}

#endif
