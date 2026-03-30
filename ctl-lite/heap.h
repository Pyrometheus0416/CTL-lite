/* Priority Queue by Maxium Root Heap */ 

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <heap.h>"
#endif

#define A JOIN(heap, T)

//-------------------------------------------------------------------
// this implicit copy method is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif

#ifndef COMPARE
#define COMPARE
static inline int default_compare(T* a, T* b) { return (*a>*b)-(*a<*b); }
#endif

//----- main structure ----------------------------------------------

typedef struct A{
    T* value;

    T (*elem_copy)(T*);
    int (*compare)(T*, T*);

    void (*up)(struct A* self, size_t n);
    void (*down)(struct A* self, size_t n);

    size_t capicity;
    size_t size;
    _Bool recursion;
} A;

static inline A JOIN(A, init)(int _compare(T*, T*), _Bool recursion);

//-------------------------------------------------------------------
static inline T*
JOIN(A, top)(A* self){ return self->value; }

static inline T*
JOIN(A, bottom)(A* self){ return self->value + (self->size -1); }

static inline void
JOIN(A, up)(A* self, size_t n){
    if(n==0 || n>= self->size) return;
    
    size_t p = (n - 1)>>1;   // get parent node index
    T *x = &self->value[n];  // get current node
    T *y = &self->value[p];  // get parent node
    if(self->compare(x, y) > 0){// x is more priority than y ?
        SWAP(T, x, y);        // x become the parent node of y
        JOIN(A, up)(self, p); // x try to go up again...
    }
}

static inline void
JOIN(A, down)(A* self, size_t n){
    if(2*n+1>= self->size) return; // no left child, and right child
    T *x = &self->value[n]; // get current node
    
    size_t l = 2*n+1; // get left child node index
    size_t r = 2*n+2; // get right child node index
    
    T *a = &self->value[l];   // get left node
    
    if(r>=self->size) {         // only left node need to check
        if(self->compare(a,x)>0)  // a is more priority than x ?
        SWAP(T, a, x); // x become the left child node of a
        return;        // we have checked all
    }
    
    T *b = &self->value[r]; // find the most priority node
    T *c = self->compare(a,b)>0?a:b; // choose the candidate c from children node
    if(self->compare(c,x)>0){        // c is really more priority than x?
        SWAP(T, c, x);               // let c become the parent node
        JOIN(A, down)(self, c-self->value);    // x try to go down again...
    } // if c isn't more priority than x, x is the maxium node. nothing need to do.
}

static inline void
JOIN(A, up_)(A* self, size_t n){// a version without recursion
    if(n==0 || n>self->size-1) return; // no parent node or out of range
    T *x = self->value+n, *y; // get current node, set parent node pointer
    T *min = x; // set minium node is n by default

    do{
        x = min; // set current node is minium by default
        size_t p = (n - 1)>>1; // get parent node index
        y = self->value+p;
        if(self->compare(y,x)<0){
            min = y; n = p;
        }
        SWAP(T, min, x);
        // swap *min and *x, then we will try to change x to pointer min
    }while(x!=min && n!=0);
}

static inline void
JOIN(A, down_)(A* self, size_t n){// a version without recursion
    if(2*n+1>self->size-1) return; // no left child node, and right child
    T *x = self->value+n, *a, *b; // get current node, set children nodes pointer
    T *max = x; // set maxium node is n by default

    do{
        x = max; // set current node is maxium by default
        size_t l = 2*n+1; // get left child node index
        size_t r = 2*n+2; // get right child node index
        if( l<=self->size-1){
            a = self->value+l;
            if(self->compare(a,x)>0){
                max = a; n = l;
            }
        }
        if( r<=self->size-1 ){
            b = self->value+r;
            if(self->compare(b,x)>0){
                max = b; n = r;
            }
        }

        SWAP(T, max,x); // x maybe swap with itself, but only one time
        // swap *max and *x, then we will try to change x to pointer max
    }while(x!=max && 2*n+1<=self->size-1);
    // when x is the maxium or no anyone child nodes, loop end
}

//-------------------------------------------------------------------
static inline void
JOIN(A, push)(A* self, T value){
    if(self->capicity==self->size){
        T* top = JOIN(A,top)(self);
        size_t new_capicity = self->capicity + 5;
        self->value = (T*)realloc(top, new_capicity*sizeof(T));
        self->capicity = new_capicity;
    }
    self->value[self->size++] = value;
    self->up(self, self->size - 1);
}

static inline void
JOIN(A, pop)(A* self, T* cache){
    if( !cache || self->size == 0 ) return;
    *cache = *(JOIN(A, top)(self));
    SWAP(T, JOIN(A, top)(self), JOIN(A, bottom)(self));
    self->size--;
    self->down(self, 0);
}

//-------------------------------------------------------------------

static inline A
JOIN(A, init)(int _compare(T*, T*), _Bool recursion){
    A self;
    self.value = (T*)calloc(5, sizeof(T));
    self.size = 0;
    self.capicity = 5;
    self.recursion = recursion;

    self.compare = _compare? _compare: default_compare;
    self.up = recursion?JOIN(A, up):JOIN(A, up_);
    self.down = recursion?JOIN(A, down):JOIN(A, down_);

#ifdef EXPLICIT
#undef EXPLICIT
    self.elem_copy = JOIN(T, copy);
#else
    self.elem_copy = JOIN(A, implicit_copy);
#endif

    return self;
}

static inline void
JOIN(A, free)(A* self){
    if(self->size)
        free(self->value);
    else
        FreeWarning;
}

static inline A
JOIN(A, copy)(A* self){
    A other = JOIN(A, init)(self->compare, self->recursion);
    for(size_t i=0; i<self->size; i++)
        JOIN(A, push)(&other, self->elem_copy(self->value+i));
    return other;
}

//-------------------------------------------------------------------

#undef T
#undef A