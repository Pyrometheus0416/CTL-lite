/* Array */ 

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <array.h>"
#endif

#define A JOIN(arr, T) // Container Instance
#define AO JOIN3(arr, T, WellOrder)   // Order Mixin
#define AH JOIN3(arr, T, Hashability) // Hash Mixin
#define Z JOIN(A, it)  // Container Iterator

//-------------------------------------------------------------------
// this implicit copy method is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif
//----- mixin structure ---------------------------------------------
typedef struct A A; // Structure forward declaration

#ifdef ORDERED
typedef struct AO{
    short (*compare)(T*, T*);
    void (*sort)(A* self, size_t l, size_t r);
    size_t (*bSearch)(A* self, size_t l, size_t r, T t);
} AO;
#endif

#ifdef HASH
typedef struct AH{
    uint64_t (*hash)(T* x);
    bool (*equal)(T* a, T* b);
} AH;
#endif
//----- main structure ----------------------------------------------

typedef struct A{
    T* value;
    T* (*at)(struct A* self, size_t index);
#ifdef ORDERED
    AO order;
#endif
#ifdef HASH
    AH hashable;
#endif

    T (*elem_copy)(T*);
    T (*elem_free)(T*);

    size_t size;
    size_t capacity; // will keep bigger than size whenever
    // [0~size) is used, [size, capacity) is available
} A;

static inline A JOIN(A, init)();
static inline void JOIN(A, push)(A* self, T value);
static inline void JOIN(A, pop)(A* self, T* cache);

//----- view function -----------------------------------------------

static inline T*
JOIN(A, at)(A* self, size_t index){ return self->value + index; }

static inline T*
JOIN(A, head)(A* self){ return self->value; }

static inline T*
JOIN(A, tail)(A* self){ return self->value + (self->size -1); }

static inline T*
JOIN(A, begin)(A* self){ return self->value; }

static inline T*
JOIN(A, end)(A* self){ return self->value + self->size; }

//----- iterator ----------------------------------------------------

typedef struct Z{
    void (*step)(struct Z*);
    T *ref, *begin, *next, *end;
    size_t stride;
    bool done;
} Z;

static void
JOIN(Z, step)(Z* self){
    if(self->next >= self->end) self->done = 1;
    if(self->next < self->begin) self->done = 1;
    if(self->done) return;

    self->ref = self->next;
    self->next += self->stride;
}

static Z
JOIN(Z, range)(A* v, size_t begin, size_t end, size_t stride){
    if(begin >= v->size) return (Z){.done=1};
    if(end > v->size) return (Z){.done=1};
    if(end <= begin) return (Z){.done=1};

    T* head = v->value;
    T* a= JOIN(A, at)(v, begin);
    T* b= JOIN(A, at)(v, end);
    return (Z){ JOIN(Z, step), head, a, a+stride, b, stride, 0};
}

static Z
JOIN(Z, each)(A* a){
    return JOIN(Z, range)(a, 0, a->size, 1);
}

//----- memory operation --------------------------------------------

static void
JOIN(A, reserve)(A* self, const size_t capacity){
    if(capacity == self->capacity) return;
    
    size_t new_capacity = capacity;

    // smartly adjust the capacity of array
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
JOIN(A, fit)(A* self){ JOIN(A, reserve)(self, self->size + 1); }

//----- modify operations -------------------------------------------

static void
JOIN(A, set)(A* self, size_t index, T value){
    T* ref = JOIN(A, at)(self, index);
    if(self->elem_free != NULL) self->elem_free(ref);
    *ref = self->elem_copy(&value);
}

static void
JOIN(A, pop)(A* self, T* cache){
    if( self->size == 0 ) return;
    if( cache ) *cache = self->elem_copy(JOIN(A, tail)(self));
    if( self->elem_free ) self->elem_free(JOIN(A, tail)(self)); 
    self->size--;
}

static void
JOIN(A, push)(A* self, T value){
    if(self->size + 1 >= self->capacity)
        JOIN(A, reserve)(self, 2 * self->capacity);
    *JOIN(A, end)(self) = self->elem_copy(&value);
    self->size++;
}


static void
JOIN(A, insert)(A* self, size_t index, T value){
    if(self->size > 0){ // auto memory
        if(self->size + 1 >= self->capacity)
            JOIN(A, reserve)(self, 2 * self->capacity);
        for(size_t i = self->size; i > index; i--)
            self->value[i] = self->value[i-1];
        self->value[index] = self->elem_copy(&value);
        self->size++;
    }else
        JOIN(A, push)(self, value);
}

static void
JOIN(A, extend)(A* self, size_t n, T arr[n]){
    for(int i=0; i<n; i++){ JOIN(A, push)(self, arr[i]);}
}

static void
JOIN(A, erase)(A* self, size_t index){
    if(self->elem_free != NULL)
        self->elem_free(JOIN(A, at)(self, index));
    for(size_t i = index; i < self->size - 1; i++){
        self->value[i] = self->value[i + 1];
    }
    self->size--;
}

//----- hign level function -----------------------------------------

#ifdef ORDERED
/* sort the range [l,r] with quick sort algorithm */
void JOIN(A, sort)(A* self, size_t l, size_t r){
    if(l+1 >= r+1) return;

    T* x = JOIN(A, at)(self, l);
    size_t i=l-1, j=r+1; // size_t >=0 !!!
    while(i+1<j+1){
        do ++i; while( self->order.compare(JOIN(A, at)(self, i), x)<0);
        do --j; while( self->order.compare(JOIN(A, at)(self, j), x)>0);
        if(i+1<j+1) SWAP(T, JOIN(A, at)(self, i), JOIN(A, at)(self, j));
    }

    JOIN(A, sort)(self, l, i - 1);
    JOIN(A, sort)(self, j + 1, r);
}

/* the final result of while loop:
index:     __________r__l___________________
condition: [T, T, T, T, F, F, F, F, F, F, F]
note: l may be self.size and r may be -1 !!!
*/
size_t JOIN(A, bSearch)(A* self, size_t l, size_t r, T t){
    size_t mid; T* mid_elem;
    while(l+1<=r+1){
        mid = l+r>>1;
        mid_elem = JOIN(A, at)(self, mid);
        if(self->order.compare(mid_elem, &t)<0) // condition
            l = mid+1;
        else
            r = mid-1;
    }
    return l; // r = l-1
}
#endif

#ifdef HASH
static bool
JOIN(A, equal)(A* self, A* other){
    if(self->size != other->size) return 0;

    Z a = JOIN(Z, each)(self);
    Z b = JOIN(Z, each)(other);
    while(!a.done && !b.done){
        if( !self->hashable.equal(a.ref, b.ref) )
            return false;
        a.step(&a);
        b.step(&b);
    }
    return true;
}

/**
 @brief Find the n-th element that equals to key.
 @param self Pointer to the array instance.
 @param key The key to search for.
 @param nth Number of matched elements
 @return Index of the found element, or self->size if not found (or skip exceeds total matches).
 
 ---
 ```
 array_int arr;
 int key = 42;
 size_t idx = array_int_find(&arr, key, 0); // find first
 if (idx != arr.size) {
    printf("Found at %zu\n", idx);
 } else {
    printf("Not found\n");
 }
 ```
*/
static size_t
JOIN(A, find)(A* self, T key, size_t nth){
    for(int index=0; index<self->size; index++)
        if(self->hashable.equal(JOIN(A, at)(self, index), &key))
            if( !nth) return index;
            else nth--;
    return self->size;
}
#endif


static size_t
JOIN(A, filter)(A* self, bool _match(T*)){
    size_t cnt = 0, index = 0, bench = 0;
    // [bench,index) will be erase as expected

    foreach(A, self, iter){
        if(_match(iter.ref)){
            SWAP(T, JOIN(A, at)(self, bench), iter.ref);
            bench++; cnt++; // matched element sit on bench,
        } // the empty seat on the bench move back
        index++;
    }
    for(; index > bench; index--){ JOIN(A, pop)(self, NULL); }
     // the tail element is in index-1 in fact
    return cnt;
}

static void
JOIN(A, reverse)(A* self, size_t l, size_t r){
    while(l<r){
        SWAP(T, JOIN(A, at)(self,l), JOIN(A, at)(self, r));
        l++; r--;
    }
}

//----- init method -------------------------------------------------

static A
JOIN(A, init)(){
    A self;
    self.value = (T*)calloc(8, sizeof(T));
    self.at = JOIN(A, at);
    self.capacity = 8;
    self.size = 0;
    
#ifdef ORDERED
    self.order = (AO){ JOIN(T, compare), JOIN(A,sort), JOIN(A,bSearch) };
#endif

#ifdef HASH
    self.hashable = (AH){JOIN(T, hash), JOIN(T, equal)};
#endif


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

static void
JOIN(A, free)(A* self){
    if(self->elem_free != NULL)
        foreach(A, self, iter){
            self->elem_free(iter.ref);
        }
    free(self->value);
    self->value = NULL;
    self->capacity = self->size = 0;
}

static A
JOIN(A, copy)(A* self){

    A other = JOIN(A, init)();
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

#ifdef ORDERED
#undef AO
#endif
#ifdef HASH
#undef AH
#endif