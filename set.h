/* Unordered Set, Open Hashing */

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <set.h>"
#endif
#ifndef HASH
#error "Template type T is not a HASHABLE type! please #define HASH!"
#endif

#define A JOIN(set, T) // Container Instance
#define AH JOIN3(set, T, Hashability) // Hash Mixin
#define B JOIN(A, node) // Chain Node for Data
#define Z JOIN(A, it)  // Container Iterator

//-------------------------------------------------------------------
// this implicit copy method is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif

static size_t
splitmix32(uint64_t x) {
    x += 0x9e3779b9;
    x = (x ^ (x >> 16)) * 0x85ebca6b;
    x = (x ^ (x >> 13)) * 0xc2b2ae35;
    return x ^ (x >> 16);
}

static uint64_t
splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
}

static size_t
closest_prime(size_t number){
    static uint32_t primes[] = {
        2, 3, 5, 7, 11,
        13, 17, 19, 23, 29, 31,
        37, 41, 43, 47, 53, 59,
        61, 67, 71, 73, 79, 83,
        89, 97, 103, 109, 113, 127,
        137, 139, 149, 157, 167, 179,
        193, 199, 211, 227, 241, 257,
        277, 293, 313, 337, 359, 383,
        409, 439, 467, 503, 541, 577,
        619, 661, 709, 761, 823, 887,
        953, 1031, 1109, 1193, 1289, 1381,
        1493, 1613, 1741, 1879, 2029, 2179,
        2357, 2549, 2753, 2971, 3209, 3469,
        3739, 4027, 4349, 4703, 5087, 5503,
        5953, 6427, 6949, 7517, 8123, 8783,
        9497, 10273, 11113, 12011, 12983, 14033,
        15173, 16411, 17749, 19183, 20753, 22447,
        24281, 26267, 28411, 30727, 33223, 35933,
        38873, 42043, 45481, 49201, 53201, 57557,
        62233, 67307, 72817, 78779, 85229, 92203,
        99733, 107897, 116731, 126271, 136607, 147793,
        159871, 172933, 187091, 202409, 218971, 236897,
        256279, 277261, 299951, 324503, 351061, 379787,
        410857, 444487, 480881, 520241, 562841, 608903,
        658753, 712697, 771049, 834181, 902483, 976369,
        1056323, 1142821, 1236397, 1337629, 1447153, 1565659,
        1693859, 1832561, 1982627, 2144977, 2320627, 2510653,
        2716249, 2938679, 3179303, 3439651, 3721303, 4026031,
        4355707, 4712381, 5098259, 5515729, 5967347, 6456007,
        6984629, 7556579, 8175383, 8844859, 9569143, 10352717,
        11200489, 12117689, 13109983, 14183539, 15345007, 16601593,
        17961079, 19431899, 21023161, 22744717, 24607243, 26622317,
        28802401, 31160981, 33712729, 36473443, 39460231, 42691603,
        46187573, 49969847, 54061849, 58488943, 63278561, 68460391,
        74066549, 80131819, 86693767, 93793069, 101473717, 109783337,
        118773397, 128499677, 139022417, 150406843, 162723577, 176048909,
        190465427, 206062531, 222936881, 241193053, 260944219, 282312799,
        305431229, 330442829, 357502601, 386778277, 418451333, 452718089,
        489790921, 529899637, 573292817, 620239453, 671030513, 725980837,
        785430967, 849749479, 919334987, 994618837, 1076067617, 1164186217,
        1259520799, 1362662261, 1474249943, 1594975441, 1725587117,
    }; // 223 prime numbers
    if( number<=primes[0]) return primes[0];
    if( number>primes[223]) return primes[223];

    size_t l=0, r=223, mid;
    while(l<r){
        mid = l+r>>1;
        if(primes[mid]<=number) l = mid+1;
        else r = mid-1;
    }
    if(primes[l]==number) return primes[l];
    else return primes[r];
}

//----- main structure ----------------------------------------------

typedef struct AH{
    uint64_t (*hash)(T* x);
    bool (*equal)(T* a, T* b);
}AH;

typedef struct B{
    T key;
    struct B* next;
} B;

typedef struct A{
    B** buckets; // A array for buckets that store B with pointer
    AH hashable; // splitmix64(hash(data)) % bucket_count --> bucket index

    void (*elem_free)(T*);
    T (*elem_copy)(T*);

    size_t size;
    size_t bucket_count;
} A;

static A JOIN(A, init)();
static void JOIN(A, reserve)(A* self, size_t desired_count);

static inline B* JOIN(B, init)(T value);

//----- view function -----------------------------------------------

static inline size_t
JOIN(A, next_bucket)(A* self, size_t index){
    for(size_t i = index; i < self->bucket_count; i++){
        if( self->buckets[i] ) return i;
    }
    return -1;
}

static inline size_t
JOIN(A, index)(A* self, T value){
    return splitmix32(self->hashable.hash(&value)) % self->bucket_count;
}

static inline float JOIN(A, load_factor)(A* self) {
    return (float)self->size / self->bucket_count;
}

static void JOIN(A, try_grow)(A* self) {
    if (JOIN(A, load_factor)(self) > 0.75f) {
        JOIN(A, reserve)(self, self->bucket_count * 2);
    }
}

//----- iterator ----------------------------------------------------

typedef struct Z{
    void (*step)(struct Z*);
    T* ref;
    
    A* container;
    B* node; // Pointer to the current node in the container
    B* next; // Pointer to the next node in the container
    size_t curr_index; // Current index of the bucket
    size_t next_index; // Next index of the bucket with element
    // define the end index is (size_t)(-1) = 4294967295
    
    bool done;
} Z;

static inline void
JOIN(Z, step)(Z* self){
    A* set = self->container;
    if(self->next == NULL){ // This bucket has been exhausted
        if(self->next_index == -1){ // All buckets have been exhausted
            self->done = 1;
        }else{ // go to next bucket and search the new next bucket
            self->curr_index = self->next_index;
            self->next_index = JOIN(A, next_bucket)(set, self->curr_index+1);
            self->node = set->buckets[self->curr_index];
            self->ref = &(self->node->key);
            self->next = self->node->next;
        }
    }else{ // Get next node in the current bucket
        self->node = self->next;
        self->ref = &self->node->key;
        self->next = self->node->next;
    }
}

static inline Z
JOIN(Z, each)(A* a){
    if( a->size == 0 )
        return (Z){.done = 1};
    
    size_t curr_index = JOIN(A, next_bucket)(a, 0);
    size_t next_index = JOIN(A, next_bucket)(a, curr_index+1);
    B* node = a->buckets[curr_index];
    T* ref = &node->key;
    return (Z){JOIN(Z, step), ref, a, node, node->next, curr_index, next_index, 0};
}

//----- memory operation --------------------------------------------

static B*
JOIN(A, squeeze)(A* self){
    B *store_p = NULL;
    B *p = NULL, *q = NULL;

    size_t begin = JOIN(A, next_bucket)(self, 0);
    for(size_t i=begin; i<self->bucket_count;){
        p = store_p;
        store_p = self->buckets[i];
        q = store_p;
        while(q->next) q = q->next;
        q->next = p;
        i = JOIN(A, next_bucket)(self, i+1);
    }
    return store_p;
} // move all element to store

static void
JOIN(A, reserve)(A* self, size_t desired_count){
    B* store_p = NULL;
    if(self->size != 0 ) store_p = JOIN(A, squeeze)(self);
    size_t bucket_count = closest_prime(desired_count);
    self->buckets = (B**)realloc(self->buckets, bucket_count*sizeof(B*));
    for (size_t i = 0; i < bucket_count; i++) self->buckets[i] = NULL;
    self->bucket_count = bucket_count;

    if( self->size == 0 ) return;
    B* current = store_p;
    while (current) {
        B* next = current->next;
        size_t index = JOIN(A, index)(self, current->key);
        current->next = self->buckets[index];
        self->buckets[index] = current;
        current = next;
    }
}

static void
JOIN(A, link_free)(A* self, B* node){
    if(node->next) JOIN(A, link_free)(self, node->next);
    if(self->elem_free) self->elem_free(&(node->key));
    free(node);
}

//----- modify operations -------------------------------------------

static void
JOIN(A, add)(A* self, T value){
    size_t index = JOIN(A, index)(self, value);

    B* data_p = self->buckets[index];
    while(data_p){
        if(self->hashable.equal(&(data_p->key), &value))
            return;
        data_p = data_p->next;
    }
    
    B* tmp = self->buckets[index];
    B* node_p = JOIN(B, init)(self->elem_copy(&value));
    self->buckets[index] = node_p;
    node_p->next = tmp;
    self->size++;
}

static void
JOIN(A, update)(A* self, size_t n, T arr[n]){
    for(size_t i=0; i<n; i++){
        JOIN(A, add)(self, arr[i]);
    }
    JOIN(A, try_grow)(self);
}

static void
JOIN(A, pop)(A* self, T* cache){
    if( self->size == 0 ) return;

    size_t index = JOIN(A, next_bucket)(self, 0);
    T* data_p = &(self->buckets[index]->key);
    if( cache ) *cache = self->elem_copy(data_p);
    if( self->elem_free ) self->elem_free(data_p);

    B* tmp = self->buckets[index]->next;
    free(self->buckets[index]);
    self->buckets[index] = tmp;
    self->size--;
}

static void
JOIN(A, remove)(A* self, T value){
    if (self->size == 0) return;

    size_t index = JOIN(A, index)(self, value);

    B* data_p = self->buckets[index];
    if(!data_p) return;
    B* prev_p;
    if(self->hashable.equal(&(data_p->key), &value)){
        self->buckets[index] = data_p->next;
    }else{
        prev_p = data_p; data_p = data_p->next;
        while(data_p){
            if(self->hashable.equal(&(data_p->key), &value)){
                prev_p->next = data_p->next; 
                break;
            }
            prev_p = data_p; data_p = data_p->next;
        }
    }

    if(!data_p) return; // Not Find
    if(self->elem_free) self->elem_free(&(data_p->key));
    free(data_p);
    self->size--;
}

static inline void
JOIN(A, clear)(A* self){
    if(self->size == 0) return;
    for (size_t i = 0; i < self->bucket_count; i++) {
        if (self->buckets[i]) {
            JOIN(A, link_free)(self, self->buckets[i]);
            self->buckets[i] = NULL;
        }
    }
    self->size = 0;
}

//----- hign level function -----------------------------------------

static bool
JOIN(A, contains)(A* self, T value){
    if(self->size==0) return false;

    size_t index = JOIN(A, index)(self, value);
    B* data_p = self->buckets[index];
    while(data_p){
        if(self->hashable.equal(&(data_p->key), &value))
            return true;
        data_p = data_p->next;
    }
    return false;
}

// a ⊆ b
static bool
JOIN(A, subset)(A* a, A* b) {
    foreach(A, a, it) {
        if (!JOIN(A, contains)(b, *it.ref))
            return false;
    }
    return true;
}

static A
JOIN(A, union)(A* a, A* b) {
    A result = JOIN(A, init)();
    foreach(A, a, it) JOIN(A, add)(&result, a->elem_copy(it.ref));
    foreach(A, b, it) JOIN(A, add)(&result, b->elem_copy(it.ref));
    JOIN(A, try_grow)(&result);
    return result;
}

static A
JOIN(A, intersection)(A* a, A* b) {
    A result = JOIN(A, init)();
    foreach(A, a, it) {
        if (JOIN(A, contains)(b, *it.ref)) {
            JOIN(A, add)(&result, a->elem_copy(it.ref));
        }
    }
    JOIN(A, try_grow)(&result);
    return result;
}

static inline A
JOIN(A, difference)(A* a, A* b) {
    A result = JOIN(A, init)();
    foreach(A, a, it) {
        if (!JOIN(A, contains)(b, *it.ref)) {
            JOIN(A, add)(&result, a->elem_copy(it.ref));
        }
    }
    JOIN(A, try_grow)(&result);
    return result;
}

//----- init method -------------------------------------------------

static inline B*
JOIN(B, init)(T value){
    B* n = (B*) malloc(sizeof(B));
    n->key = value;
    n->next = NULL;
    return n;
}

static inline A
JOIN(A, init)(){
    A self;
    self.size = 0;
    self.buckets = (B**)calloc(3, sizeof(B*));
    self.bucket_count = 3;
    self.hashable = (AH){JOIN(T, hash), JOIN(T, equal)};

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

static inline A
JOIN(A, copy)(A* self){
    A other = JOIN(A, init)();
    foreach(A, self, it)
        JOIN(A, add)(&other, self->elem_copy(it.ref));
    return other;
}

static void
JOIN(A, free)(A* self){
    for(size_t i=0; i<self->bucket_count; i++){
        if( !(self->buckets[i]) ) continue;
        JOIN(A, link_free)(self, self->buckets[i]);
        self->buckets[i] = NULL;
    }
    free(self->buckets);
}

//-------------------------------------------------------------------
#undef A
#undef B
#undef Z

#undef AH