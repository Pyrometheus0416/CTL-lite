/* Priority Queue by Maxium Root Heap */ 

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <heap.h>"
#endif
#ifndef ORDERED
#error "Template type T is not a ORDERED type! please #define ORDERED!"
#endif

#define A JOIN(heap, T)
#define AO JOIN3(heap, T, order) // Order Mixin

//-------------------------------------------------------------------
// this implicit copy method is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif

//----- main structure ----------------------------------------------

typedef struct A A; // Structure forward declaration

typedef struct AO{
    short (*compare)(T*, T*);
    void (*up)(struct A* self, size_t n);
    void (*down)(struct A* self, size_t n);
} AO;

typedef struct A{
    T* value;
    AO order;

    T (*elem_copy)(T*);
    T (*elem_free)(T*);

    size_t capacity;
    size_t size;
    bool recursion;
} A;

static inline A JOIN(A, init)(short _compare(T*, T*), bool recursion);

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
    if(self->order.compare(x,y) > 0){// x is more priority than y ?
        SWAP(T, x, y);        // x become the parent node of y
        JOIN(A, up)(self, p); // x try to go up again...
    }
}

static inline void
JOIN(A, down)(A* self, size_t n){
    if(n >= self->size) return;
    T *x = &self->value[n], *a, *b; // get current node
    T *c = x; // let current node become the candidate c

    size_t l = 2*n+1; // get left child node index
    size_t r = 2*n+2; // get right child node index
    if(l <= self->size-1){
        a = &self->value[l];   // let left node become the candidate c
        c = self->order.compare(a,c)?a:c;
    }
    if(r <= self->size-1){
        b = &self->value[r];
        c = self->order.compare(b,c)?b:c;
    }

    if(self->order.compare(c,x)>0){  // c is really more priority than x?
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
        if(self->order.compare(y,x)<0){
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
            if(self->order.compare(a,x)>0){
                max = a; n = l;
            }
        }
        if( r<=self->size-1 ){
            b = self->value+r;
            if(self->order.compare(b,x)>0){
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
    if(self->capacity==self->size){
        T* top = JOIN(A,top)(self);
        size_t new_capicity = self->capacity + 5;
        self->value = (T*)realloc(self->value, new_capicity*sizeof(T));
        self->capacity = new_capicity;
    }
    self->value[self->size++] = self->elem_copy(&value);
    self->order.up(self, self->size - 1);
}

static inline void
JOIN(A, pop)(A* self, T* cache){
    if( self->size == 0 ) return;
    if( cache ) *cache = self->elem_copy(JOIN(A, top)(self));
    if( self->elem_free ) self->elem_free(JOIN(A, top)(self)); 
    SWAP(T, JOIN(A, top)(self), JOIN(A, bottom)(self));
    self->size--;
    self->order.down(self, 0);
}

//-------------------------------------------------------------------

static inline A
JOIN(A, init)(short _compare(T*, T*), _Bool recursion){
    A self;
    self.value = (T*)calloc(5, sizeof(T));
    self.size = 0;
    self.capacity = 5;
    self.recursion = recursion;

    self.order = (AO){
        .compare = JOIN(T, compare),
        .up = recursion?JOIN(A, up):JOIN(A, up_),
        .down = recursion?JOIN(A, down):JOIN(A, down_)
    };

#ifdef EXPLICIT
#undef EXPLICIT
    self.elem_copy = JOIN(T, copy);
    self.elem_free = JOIN(T, free);
#else
    self.elem_copy = JOIN(A, implicit_copy);
    self.elem_free = NULL;
#endif

    return self;
}

static inline void
JOIN(A, free)(A* self){
    if(self->elem_free != NULL)
        for(size_t i=0; i<self->size; i++)
            self->elem_free(self->value+i);
    free(self->value);
    self->value = NULL;
    self->capacity = self->size = 0;
}

static inline A
JOIN(A, copy)(A* self){
    A other = *self;
    other.value = (T*)calloc(self->capacity, sizeof(T));
    for(size_t i=0; i<self->size; i++)
        other.value[i] = self->elem_copy(self->value+i);
    return other;
}

//-------------------------------------------------------------------

#undef A
#undef AO