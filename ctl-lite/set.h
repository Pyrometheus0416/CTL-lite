/* Unordered Set, Open Hashing */

#include "ctl.h"
//-------------------------------------------------------------------
#ifndef T
#error "Template type T undefined for <set.h>"
#endif

#define A JOIN(set, T) // Container Instance
#define B JOIN(A, node) // Chain Node for Data
#define Z JOIN(A, it)  // Container Iterator

//-------------------------------------------------------------------
// this implicit copy method is prepared for int,float,char...,
// these types are built in C language.
#ifndef EXPLICIT
static inline T JOIN(A, implicit_copy)(T *data) { return *data; }
#endif

static inline size_t
splitmix32(T* p) {
    int x = *p;
    x += 0x9e3779b9;
    x = (x ^ (x >> 16)) * 0x85ebca6b;
    x = (x ^ (x >> 13)) * 0xc2b2ae35;
    return x ^ (x >> 16);
}

static inline size_t
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

typedef struct B{
    T key;
    struct B* next;
} B;

typedef struct A{
    B** buckets; // A array for buckets that store B head pointer

    void (*elem_free)(T*);
    T (*elem_copy)(T*);
    size_t (*hash)(T*); // data --hash & modulo--> bucket index
    _Bool (*equal)(T*, T*);

    size_t size;
    size_t bucket_count;
} A;

static inline A JOIN(A, init)(size_t _hash(T*), _Bool _equal(T*, T*));
static inline void JOIN(A, push)(A* self, B** bucket, B* n);
static inline B* JOIN(A, find)(A* self, T value);
static inline void JOIN(A, rehash)(A* self, size_t desired_count);

static inline B* JOIN(B, init)(T value);

//----- view function -----------------------------------------------

static inline B*
JOIN(A, begin)(A* self){
    B* node_p;
    for(size_t i = 0; i < self->bucket_count; i++){
        node_p = self->buckets[i];
        if(node_p)
            return node_p;
    }
    return NULL;
}

static inline int
JOIN(A, empty)(A* self){ return self->size == 0; }

static inline size_t
JOIN(A, index)(A* self, T value){
    return self->hash(&value) % self->bucket_count;
} // Hash function should depend on value's property, not address

static inline B**
JOIN(A, bucket)(A* self, T value){
    size_t index = JOIN(A, index)(self, value);
    return self->buckets + index;
}

static inline size_t
JOIN(A, bucket_size)(A* self, size_t index){
    size_t size = 0;
    for(B* n = self->buckets[index]; n; n = n->next)
        size ++;
    return size;
}

static inline float
JOIN(A, load_factor)(A* self){
    return (float) self->size / (float) self->bucket_count;
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
    
    _Bool done;
} Z;

static inline void
JOIN(Z, step)(Z* self){
    A* set = self->container;
    if(self->next == NULL){ // This bucket has been exhausted
        if(self->next_index == -1){ // All buckets have been exhausted
            self->done = 1;
        }else{ // go to next bucket and search the new next bucket
            self->curr_index = self->next_index;
            self->next_index = -1;
            for(size_t i = self->curr_index + 1; i < set->bucket_count; i++){
                if(set->buckets[i]){
                    self->next_index = i;
                    break;
                }
            }
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
    if( JOIN(A, empty)(a) )
        return (Z){.done = 1};
    
    B* node = JOIN(A, begin)(a);
    T* ref = &node->key;
    size_t curr_index = JOIN(A, index)(a, node->key);
    size_t next_index = -1;
    for(size_t i = curr_index + 1; i < a->bucket_count; i++){
        if(a->buckets[i]){
            next_index = i;
            break;
        }
    }
    return (Z){JOIN(Z, step), ref, a, node, node->next, curr_index, next_index, 0};
}

//----- memory operation --------------------------------------------

static inline void
JOIN(A, reserve)(A* self, size_t desired_count){
    if(!JOIN(A, empty)(self)){
        JOIN(A, rehash)(self, desired_count);
    }else{
        size_t bucket_count = closest_prime(desired_count);
        B** temp = (B**) calloc(bucket_count, sizeof(B*));
        for(size_t i = 0; i < self->bucket_count; i++)
            temp[i] = self->buckets[i];
        free(self->buckets);
        self->buckets = temp;
        self->bucket_count = bucket_count;
    }
}

static inline void
JOIN(A, rehash)(A* self, size_t desired_count){
    desired_count = MAX_CTL(desired_count, self->size+1);
    size_t expected = closest_prime(desired_count);
    if(expected == self->bucket_count) return;
    
    A rehashed = JOIN(A, init)(self->hash, self->equal);
    JOIN(A, reserve)(&rehashed, desired_count); // reserve empty set
    foreach(A, self, it){
        B** bucket = JOIN(A, bucket)(&rehashed, it.node->key);
        JOIN(A, push)(&rehashed, bucket, it.node);
    }
    free(self->buckets);
    *self = rehashed;
}

static inline void
JOIN(A, free_node)(A* self, B* n){
    if(self->elem_free)
        self->elem_free(&n->key);
    free(n);
    self->size -= 1;
}

static inline void
JOIN(A, clear)(A* self){
    foreach(A, self, it)
        JOIN(A, free_node)(self, it.node);
    for(size_t i = 0; i < self->bucket_count; i++)
        self->buckets[i] = NULL;
}

static inline void
JOIN(A, free)(A* self){
    JOIN(A, clear)(self);
    free(self->buckets);
}

//----- modify operations -------------------------------------------

static inline void
JOIN(A, push)(A* self, B** bucket, B* n){
    n->next = *bucket;
    self->size ++;
    *bucket = n;
}

static inline void
JOIN(A, add)(A* self, T value){
    if(JOIN(A, empty)(self)) JOIN(A, rehash)(self, 12);

    if(JOIN(A, find)(self, value)){
        if(self->elem_free)
            self->elem_free(&value);
    }else{
        B** bucket = JOIN(A, bucket)(self, value);
        JOIN(A, push)(self, bucket, JOIN(B, init)(value));
        if(self->size > self->bucket_count)
            JOIN(A, rehash)(self, 2 * self->bucket_count);
    }
}

static inline void
JOIN(A, linked_erase)(A* self, B** bucket, B* n, B* prev, B* next){
    JOIN(A, free_node)(self, n);
    if(prev)
        prev->next = next;
    else
        *bucket = next;
}

static inline void
JOIN(A, erase)(A* self, T value){
    if( self->size == 0 ) return;

    B** bucket = JOIN(A, bucket)(self, value);
    B* prev = NULL;
    B* n = *bucket;
    while(n){
        B* next = n->next;
        if(self->equal(&n->key, &value)){
            JOIN(A, linked_erase)(self, bucket, n, prev, next);
            break;
        }else
            prev = n;
        n = next;
    }
}

//----- hign level function -----------------------------------------

static inline B*
JOIN(A, find)(A* self, T value){
    if( JOIN(A, empty)(self) ) return NULL;

    B** bucket = JOIN(A, bucket)(self, value);
    for(B* n = *bucket; n; n = n->next)
        if(self->equal(&value, &(n->key)))
            return n;
    return NULL;
}

static inline int
JOIN(A, equal)(A* a, A* b){
    _Bool notfind_a = 0; // from b set
    _Bool notfind_b = 0; // from a set
    foreach(A, a, it) if(!JOIN(A, find)(b, *it.ref)) notfind_a = 1;
    foreach(A, b, it) if(!JOIN(A, find)(a, *it.ref)) notfind_b = 1;
    return !notfind_a && !notfind_b;
}

static inline A
JOIN(A, copy)(A* self){
    A other = JOIN(A, init)(self->hash, self->equal);
    foreach(A, self, it)
        JOIN(A, add)(&other, self->elem_copy(it.ref));
    return other;
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
JOIN(A, init)(size_t _hash(T*), _Bool _equal(T*, T*)){
    A self;
    self.size = 0;
    self.buckets = (B**)calloc(3, sizeof(B*));
    self.bucket_count = 3;
    self.hash = _hash;
    self.equal = _equal;

#ifdef EXPLICIT
#undef EXPLICIT
    self.elem_free = JOIN(T, free);
    self.elem_copy = JOIN(T, copy);
#else
    self.elem_free = NULL;
    self.elem_copy = JOIN(A, implicit_copy);
#endif

    return self;
}

//-------------------------------------------------------------------
#undef T
#undef A
#undef B
#undef Z
