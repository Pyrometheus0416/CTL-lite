/* Double Ended Queue */

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <deque.h>"
#endif

#define A JOIN(deq, T)  // Container Instance
#define B JOIN(A, page) // Array for Data
#define Z JOIN(A, it)   // Container Iterator

#ifndef DEQ_PAGE_SIZE
#define DEQ_PAGE_SIZE (4)
#endif

//-------------------------------------------------------------------
// this implicit copy mathed is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif

//----- main structure ----------------------------------------------

typedef T B[DEQ_PAGE_SIZE];

typedef struct A{
    B **pages; // pages[i] ⇒ page; *page ⇒ B; B[i] ⇒ T
    T (*elem_copy)(T *); 

    size_t prologue; // Index of first used Page
    size_t epilogue; // Index of last used Page
    size_t a; // Index of head element in prologue page
    size_t b; // Index past tail element in epilogue page (maybe)

    size_t capacity; // Number of all pages
    size_t size; // Number of All elements
} A;

static inline A JOIN(A, init)();
static inline void JOIN(A, push)(A *self, T value);
static inline void JOIN(A, pushr)(A *self, T value);
static inline void JOIN(A, pop)(A *self, T* cache);
static inline void JOIN(A, popr)(A *self, T* cache);

static inline B* JOIN(B, init)(size_t cut);

//----- view function -----------------------------------------------

static inline B*
JOIN(A, first)(A *self){ return self->pages[self->prologue]; }

static inline B*
JOIN(A, last)(A *self){ return self->pages[self->epilogue]; }

static inline T*
JOIN(A, at)(A *self, size_t index){
    if (index >= self->size) return NULL;

    B *first = JOIN(A, first)(self);
    size_t actual = index + self->a;
    size_t q = actual / DEQ_PAGE_SIZE;
    size_t r = actual % DEQ_PAGE_SIZE;
    B *page = self->pages[self->prologue + q];
    return *page+r;
}

static inline T*
JOIN(A, head)(A *self){
    return (*self->pages[self->prologue])+self->a;
}

static inline T*
JOIN(A, tail)(A *self){
    size_t z = (self->b + DEQ_PAGE_SIZE -1)%DEQ_PAGE_SIZE;
    return (*self->pages[self->epilogue]) + z;
}

//----- iterator ----------------------------------------------------

typedef struct Z{
    void (*step)(struct Z *);
    T *ref;

    A *container;
    size_t index;
    size_t index_next;
    _Bool done;
} Z;

static inline void
JOIN(Z, step)(Z *self){
    self->index = self->index_next;
    if (self->index >= self->container->size)
        self->done = 1;
    else{
        self->ref = JOIN(A, at)(self->container, self->index);
        self->index_next++;
    }
}

static inline Z
JOIN(Z, each)(A *a){
    if( a->size == 0 ) return (Z){.done=1};
    return (Z){JOIN(Z, step), JOIN(A, head)(a), a, 0, 1, 0}; // 
}

//----- memory operation --------------------------------------------

static inline void
JOIN(A, reserve)(A* self, const size_t capacity, int shift){
    if(capacity == self->capacity) return;
    
    size_t new_capacity = capacity;
    size_t used_cnt = self->epilogue - self->prologue + 1;

    // smartly adjust the capacity of deq
    if(capacity < used_cnt){
        new_capacity = used_cnt;
    } // try decrease self->capacity but keep the used page
    if(capacity > self->capacity && capacity <= 2*self->capacity){
        new_capacity = 2*self->capacity;
    } // increase self->capacity by at least twice the original amount

    if( new_capacity > self->capacity){
        self->pages = (B**)realloc(self->pages, new_capacity * sizeof(B*));
        self->capacity = new_capacity;
    } // if increase self->capacity, do it before move the pages

    size_t center_all = new_capacity / 2;
    size_t center_used = (self->prologue + self->epilogue + 1) / 2;
    size_t bound = (new_capacity - used_cnt) / 2;

    int abs = (int)center_all - (int)center_used + shift;
    if( abs > 0){
        abs = MIN_CTL(abs, bound);
        for(size_t i = self->epilogue; i>=self->prologue; i--){
            self->pages[i+abs] = self->pages[i];
            if(i == 0) break;
        }
    }else{
        abs = MAX_CTL(abs, -bound);
        for(size_t i = self->prologue; i<=self->epilogue; i++){
            self->pages[i+abs] = self->pages[i];
        }
    }
    self->prologue += abs;
    self->epilogue += abs;

    if( new_capacity < self->capacity){
        self->pages = (B**)realloc(self->pages, new_capacity * sizeof(B*));
        self->capacity = new_capacity;
    } // if decrease self->capacity, do it after move the pages
}


//----- modify operations -------------------------------------------

static inline void
JOIN(A, push)(A *self, T value){
    if( self->a == 0){
        if( self->prologue == 0)
            JOIN(A, reserve)(self, self->capacity+2, 0);
        self->prologue--;
        self->pages[self->prologue] = JOIN(B, init)(DEQ_PAGE_SIZE);
    }
    self->a = (self->a + DEQ_PAGE_SIZE - 1)%DEQ_PAGE_SIZE;
    (*JOIN(A, first)(self))[self->a] = value;
    self->size++;
}

static inline void
JOIN(A, pop)(A *self, T* cache){
    if( !cache || self->size == 0 ) return;
    *cache = *JOIN(A, head)(self);
    if( self->a == DEQ_PAGE_SIZE-1 ){
        free(JOIN(A, first)(self));
        self->prologue++;
    }
    self->a = (self->a + 1)%DEQ_PAGE_SIZE;
    self->size--;
}

static inline void
JOIN(A, pushr)(A *self, T value){
    size_t b_ = self->b;
    if( self->b == 0 ){
        if( self->epilogue == self->capacity-1 )
            JOIN(A, reserve)(self, self->capacity+2, 0);
        self->epilogue++;
        self->pages[self->epilogue] = JOIN(B, init)(DEQ_PAGE_SIZE);
    }
    self->b = (self->b + 1)%DEQ_PAGE_SIZE;
    (*JOIN(A, last)(self))[b_] = value;
    self->size++;
}

static inline void
JOIN(A, popr)(A *self, T* cache){
    if( !cache || self->size == 0 ) return;
    *cache = *JOIN(A, tail)(self);
    if( self->b == 1){
        free(JOIN(A, last)(self));
        self->epilogue--;
    }
    self->b = (self->b + DEQ_PAGE_SIZE -1)%DEQ_PAGE_SIZE;
    self->size--;
}

//----- init method -------------------------------------------------

static inline B*
JOIN(B, init)(size_t cut){
    B *self = (B*)malloc(sizeof(B));
    return self;
}

static inline A
JOIN(A, init)(){
    A self = (A){NULL, NULL, 1, 0, 0, 0, .capacity=2, .size=0};
    self.pages = (B**)calloc(self.capacity, sizeof(B*));

#ifdef EXPLICIT
#undef EXPLICIT
    self.elem_copy = JOIN(T, copy);
#else
    self.elem_copy = JOIN(A, implicit_copy);
#endif

    return self;
}

static inline void
JOIN(A, free)(A *self){
    if( self->size == 0 )
        free(self->pages);
    else
        FreeWarning;
}

static inline A
JOIN(A, copy)(A *self){
    A other = JOIN(A, init)();
    while (other.size < self->size){
        T *value = JOIN(A, at)(self, other.size);
        JOIN(A, pushr)(&other, self->elem_copy(value));
    }
    return other;
}

//-------------------------------------------------------------------

#undef T
#undef A
#undef B
#undef Z

#undef DEQ_BUCKET_SIZE