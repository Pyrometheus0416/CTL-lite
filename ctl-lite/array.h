/* Vector */ 

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <array.h>"
#endif

#define A JOIN(arr, T) // Container Instance
#define Z JOIN(A, it)  // Container Iterator

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
    T* (*at)(struct A* self, size_t index);

    size_t size;
    size_t capacity; // will keep bigger than size whenever
    // [0~size) is used, [size, capacity) is available
} A;

static inline A JOIN(A, init)(int _compare(T*, T*));
static inline void JOIN(A, push)(A* self, T value);
static inline void JOIN(A, pop)(A* self, T* cache);

//----- view function -----------------------------------------------

static inline T*
JOIN(A, at)(A* self, size_t index){ return self->value + index; }

static inline T*
JOIN(A, head)(A* self){ return self->value; }

static inline T*
JOIN(A, tail)(A* self){ return self->value + (self->size -1); }

static inline
T* JOIN(A, begin)(A* self){ return self->value; }

static inline T*
JOIN(A, end)(A* self){ return self->value + self->size; }

//----- iterator ----------------------------------------------------

typedef struct Z{
    void (*step)(struct Z*);
    T* ref;
    
    T* begin;
    T* next;
    T* end;
    size_t stride;

    _Bool done;
} Z;

static inline void
JOIN(Z, step)(Z* self){
    if(self->next >= self->end) self->done = 1;
    if(self->next < self->begin) self->done = 1;
    if(self->done) return;

    self->ref = self->next;
    self->next += self->stride;
}

static inline Z
JOIN(Z, range)(A* v, size_t begin, size_t end, size_t stride){
    if(begin >= v->size) return (Z){.done=1};
    if(end > v->size) return (Z){.done=1};
    if(end <= begin) return (Z){.done=1};

    Z self;
    T* head = v->value;
    T* a= JOIN(A, at)(v, begin);
    T* b= JOIN(A, at)(v, end);
    return (Z){ JOIN(Z, step), head, a, a+stride, b, stride, 0};
}

static inline Z
JOIN(Z, each)(A* a){
    return JOIN(Z, range)(a, 0, a->size, 1);
}

//----- memory operation --------------------------------------------

static inline void
JOIN(A, reserve)(A* self, const size_t capacity){
    if(capacity == self->capacity) return;
    
    size_t new_capacity = capacity;

    // smartly adjust the capacity of vec
    if(capacity <= self->size){
        new_capacity = self->size + 1;
    } // decrease self->capacity but keep the used range
    if(capacity > self->capacity && capacity <= 2*self->capacity){
        new_capacity = 2*self->capacity;
    } // increase self->capacity by at least twice the original amount

    self->value = (T*)realloc(self->value, new_capacity * sizeof(T));
    self->capacity = new_capacity;
}

static inline void
JOIN(A, fit)(A* self){
    JOIN(A, reserve)(self, self->size + 1);
}

//----- modify operations -------------------------------------------

static inline T
JOIN(A, set)(A* self, size_t index, T value){
    T* ref = JOIN(A, at)(self, index);
    T cache = *ref;
    *ref = value;
    return cache;
}

static inline void
JOIN(A, pop)(A* self, T* cache){
    if( !cache || self->size == 0 ) return;
    *cache = *JOIN(A, tail)(self);
    self->size--;
}

static inline void
JOIN(A, push)(A* self, T value){
    if(self->size + 1 >= self->capacity)
        JOIN(A, reserve)(self, 2 * self->capacity);
    *JOIN(A, end)(self) = value;
    self->size++;
}

//----- hign level function -----------------------------------------

static inline bool
JOIN(A, equal)(A* self, A* other){
    if(self->size != other->size) return 0;

    Z a = JOIN(Z, each)(self);
    Z b = JOIN(Z, each)(other);
    while(!a.done && !b.done){
        if(!self->compare(a.ref, b.ref))
            return 0;
        a.step(&a);
        b.step(&b);
    }
    return 1;
}

static inline void
JOIN(A, insert)(A* self, size_t index, T value){
    if(self->size > 0){ // auto memory
        JOIN(A, push)(self, *JOIN(A, tail)(self));
        for(size_t i = self->size - 2; i >= index; i--)
            self->value[i+1] = self->value[i];
        self->value[index] = value;
    }else
        JOIN(A, push)(self, value);
}

static inline T
JOIN(A, erase)(A* self, size_t index){
    int cache = *JOIN(A, at)(self, index);
    for(size_t i = index; i < self->size - 1; i++){
        self->value[i] = self->value[i + 1];
    }
    self->size--;
    return cache;
}

/* sort the range [l,r] with quick sort algorithm */
static inline void
JOIN(A, ranged_sort)(A* self, int64_t l, int64_t r){
    if(l >= r) return;

    T* x = JOIN(A, at)(self, l);
    int64_t i=l-1, j=r+1;
    while(i<j){
        do ++i; while( self->compare(JOIN(A, at)(self, i), x)<0 );
        do --j; while( self->compare(JOIN(A, at)(self, j), x)>0);
        if(i<j) SWAP(T, JOIN(A, at)(self, i), JOIN(A, at)(self, j));
    }

    JOIN(A, ranged_sort)(self, l, i - 1);
    JOIN(A, ranged_sort)(self, j + 1, r);
}

static inline void
JOIN(A, sort)(A* self){
    JOIN(A, ranged_sort)(self, 0, self->size - 1);
}

/* the final result of while loop:
index:     __________r__l___________________
condition: [T, T, T, T, F, F, F, F, F, F, F]
note: l may be self.size and r may be -1 !!!
*/
static inline size_t
JOIN(A, bSearch)(A* self, size_t l, size_t r, T* t){
    size_t mid; T* mid_elem;
    while(l<=r){
        mid = l+r>>1;
        mid_elem = JOIN(A, at)(self, mid);
        if(self->compare(mid_elem, t)<0) // condition
            l = mid+1;
        else
            r = mid-1;
    }
    return l; // r = l-1
}


static inline size_t
JOIN(A, filter)(A* self, _Bool _match(T*)){
    size_t cnt = 0, index = 0, bench = 0;
    // [bench,index) will be erase as expected

    foreach(A, self, iter){
        if(_match(iter.ref)){
            SWAP(T, JOIN(A, at)(self, bench), iter.ref);
            bench++; cnt++; // matched element sit on bench,
        } // the empty seat on the bench move back
        index++;
    }
    for(; index > bench; index--) JOIN(A, pop)(self, NULL);
     // the tail element is in index-1 in fact
    return cnt;
}

static inline T*
JOIN(A, find)(A* self, T key){
    foreach(A, self, it)
        if(self->compare(it.ref, &key)==0)
            return it.ref;
    return NULL;
}

static inline void
JOIN(A, ranged_reverse)(A* self, int64_t l, int64_t r){
    while(l<r){
        SWAP(T, JOIN(A, at)(self,l), JOIN(A, at)(self, r));
        l++; r--;
    }
}

static inline void
JOIN(A, reverse)(A* self){
    JOIN(A, ranged_reverse)(self, 0, self->size-1);
}

//----- init method -------------------------------------------------

static inline A
JOIN(A, init)(int _compare(T*, T*)){
    A self;
    self.value = (T*)calloc(8, sizeof(T));
    self.capacity = 8;
    self.size = 0;
    self.compare = _compare? _compare: default_compare;
    
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
    if( self->size )
        free(self->value);
    else
        FreeWarning;
}

static inline A
JOIN(A, copy)(A* self){
    A other = JOIN(A, init)(self->compare);
    T buffer;
    JOIN(A, reserve)(&other, self->size+1);
    while(!self->size)
        buffer = other.elem_copy(JOIN(A, at)(self, other.size));
        JOIN(A, push)(&other, buffer);
    return other;
}

//-------------------------------------------------------------------
#undef A
#undef Z
#undef T