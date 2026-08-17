/* String */ 

#include <string.h>
#include "ctl.h"
#pragma once
//-------------------------------------------------------------------

typedef struct str{

    size_t len;
    size_t capacity; // assert: capacity > len;
    // [0~len) is used, [len, capacity) is available

    char* c_str; // C-style null-terminated string
    // assert: c_str[len] == '\0';
}str;

// [l,r)
typedef struct slice{
    size_t l,r;
}slice;

//-------------------------------------------------------------------

static inline str
str_init(const char* c_str){
    str self;
    self.len = strlen(c_str);
    self.capacity = self.len + 15;
    self.c_str = (char*)malloc(self.capacity);
    strcpy(self.c_str, c_str);
    return self;
}

static inline void
str_free(str* self){
    memset(self->c_str, '\0', self->capacity);
    free(self->c_str);
    self->capacity = self->len = 0;
}

static inline str
str_copy(str* s){
    str other = str_init(s->c_str);
    return other;
}

//-------------------------------------------------------------------

static inline char*
str_at(str* self, size_t index){
    index = MIN_CTL(index, self->len-1);
    return self->c_str + index;
}

static inline char*
str_head(str* self){ return self->c_str; }

static inline char*
str_tail(str* self){ return self->c_str + (self->len -1); }

//----- memory operation --------------------------------------------

static inline void
str_reserve(str* self, const size_t capacity){
    if(capacity == self->capacity) return;
    
    size_t new_capacity = capacity;

    // smartly adjust the capacity of str
    if(capacity <= self->len){
        new_capacity = self->len + 1;
    } // try decrease capacity but keep the used range
    if(capacity > self->capacity && capacity <= 2*self->capacity){
        new_capacity = 2*self->capacity;
    } // increase capacity by at least twice the original amount

    self->c_str = (char*)realloc(self->c_str, new_capacity);
    self->capacity = new_capacity;
}

static inline void
str_fit(str* self){ str_reserve(self, self->len + 1); }

//----- modify operations -------------------------------------------


static inline void str_set(str* self, slice lr, const char ch){
    if( lr.l >= lr.r) return;
    memset(self->c_str+lr.l, ch, lr.r-lr.l); 
}

static inline short str_cmp(str* self, str* other){ strcmp(self->c_str, other->c_str); }
static inline void str_upper(str* self){ strupr(self->c_str); }
static inline void str_lower(str* self){ strlwr(self->c_str); }

static inline void str_cat(str* self, str* other){
    size_t new_len = self->len + other->len;
    if( new_len >= self->capacity ) str_reserve(self, new_len+1);
    strcat(self->c_str, other->c_str);
    self->len = new_len;
}

static inline void str_reverse(str* self, slice lr){
    char* lp = self->c_str + lr.l;
    char* rp = self->c_str + lr.r-1;
    while( lp < rp){
        SWAP(char, lp, rp);
        lp++; rp--;
    }
}

static inline void str_stride(str* self){
    if(self->len==0) return;
    size_t lp = 0, rp = self->len-1;
    char ch;
    while(lp<=rp){
        ch = self->c_str[lp];
        if( 32 < ch && ch < 127) break;
        lp++;
    }
    while(lp<=rp){
        ch = self->c_str[rp];
        if( 32 < ch && ch < 127) break;
        rp--;
    }
    for(size_t i=lp; i<=rp; i++){
        self->c_str[i-lp] = self->c_str[i];
    }
    self->len = rp-lp+1;   
    self->c_str[self->len] = '\0';
}

static inline void
str_replace(str* self, slice lr, const char* s){
    char tmp[strlen(s)+1];
    strcpy(tmp, s);
    lr.r = MIN_CTL(lr.r, self->len);
    size_t erase_len = (lr.r>lr.l)? lr.r-lr.l: 0;
    size_t reserve_len = self->len-erase_len;
    size_t final_len = reserve_len + strlen(s);
    strrev(self->c_str + lr.l); strrev(tmp); // move erase char back
    if( final_len >= self->capacity ) str_reserve(self, final_len+1);
    strcpy(self->c_str + reserve_len, tmp);  // cover erase char
    strrev(self->c_str + lr.l); strrev(tmp); // resume order
    self->len = final_len;
    self->c_str[self->len] = '\0';
}

//----- hign level function -----------------------------------------

static inline void
kmp_next(str* pat, size_t* next){
    size_t M = pat->len;
    memset(next, 0, M*sizeof(size_t)); // initialize the array with 0

// In the `next` array, we offset the indices by one (Offset Trick):
// When we want to backtrack j, use j=next[j-1] instead of j=next[j]
// With this change, the next array no longer indicates the position
// to backtrack when happend a mismatch.
// Instead, it stores the length of the Longest Common Prefix and Suffix
// for the pat substring [0..j].

    // build the next array, 
    for(size_t i=1, j=0; i<M; i++){
        if(pat->c_str[i]==pat->c_str[j]){
            next[i] = j + 1; j++;
        }else if(j>0){
            j = next[j-1]; i--;
            // try to find shorter [prefix == suffix], Hold on i
        }
    }

    for(size_t i=1, j=0; i<M; j=next[i], i++){
        if( j==0 ) continue;
        if(pat->c_str[j] == pat->c_str[i])
            next[i-1] = next[j-1];
    } // nextval array
}

static inline slice
str_kmp(str* txt, str* pat, size_t* next){
    size_t N = txt->len, M = pat->len;

    // Here, j represents the length of pat
    // that has already been successfully matched.
    for(size_t i=0, j=0; i<N; i++){
        if(txt->c_str[i]==pat->c_str[j]){
            j++;
        }else if(j>0){
            j = next[j-1]; i--; // Hold on i
        }

        if(j==M) // BingGo!!! successfully match!
            return (slice){i+1-j ,i+1};
    }
    return (slice){0, 0};
}