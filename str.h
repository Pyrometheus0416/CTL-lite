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

static inline void str_concat(str* self, const char* s);

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
    self->len = 0;
    self->capacity = 15;
    self->c_str = (char*)malloc(self->capacity);
}

static inline str
str_copy(str* s){
    str other = str_init("");
    str_concat(&other, s->c_str);
    return other;
}

//-------------------------------------------------------------------

static inline char*
str_at(str* self, size_t index){ return self->c_str + index; }

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

static inline void
str_erase(str* self, size_t index, size_t len){
    char* p = str_at(self, index);
    const size_t move_cnt = self->len - index;
    for(int i = 0; i<move_cnt; i++)
        p[i] = p[i+len];
    self->len -= len;
    self->c_str[self->len] = '\0';
}

static inline void
str_fill(str* self, size_t index, size_t len){
    str_reserve(self, self->capacity + len);
    char* p = str_at(self, index);
    const size_t move_cnt = self->len - index;
    for(int i=move_cnt; i>0; --i){
        p[i-1+len] = p[i-1];
    }
    self->len += len;
    self->c_str[self->len] = '\0';
} // fill some useless char to c_str[index]

static inline void
str_insert(str* self, size_t index, const char* s){
    size_t len = strlen(s);
    char* p = str_at(self, index);
    str_fill(self, index, len);
    for(int i=0; i<len; ++i){
        p[i] = s[i];
    } // src string copy to head
    self->len += len;
    self->capacity += len;
    self->c_str[self->len] = '\0';
}

static inline void
str_concat(str* self, const char* s){
    size_t len = strlen(s);
    str_reserve(self, self->capacity + len);
    strcat( self->c_str, s);
    self->len += len;
    self->capacity += len;
}

static inline void
str_clear(str* self){
    strset(self->c_str, '\0');
    self->len = 0;
}

static inline void
str_push(str* self, const char c){
    str_reserve(self, self->capacity+1);
    *(str_tail(self)+1) = c;
    self->len++;
    *(str_tail(self)+1) = '\0';
}

static inline void
str_pop(str* self, char* c){
    *c = *(str_tail(self));
    *str_tail(self) = '\0';
    self->len--;
}

static inline void
str_reverse(str* self, size_t index, size_t len){
    char* start = self->c_str+index;
    for(size_t i = 0; 2*i<len; i++){
        char* lp = start+i;
        char* rp = start+len-i;
        SWAP(char, lp, rp);
    }
}

//----- hign level function -----------------------------------------

static inline void
str_replace(str* self, size_t index, size_t len, const char* s){
    size_t end = index + len, s_len = strlen(s);
    if( s_len < len){ // self->len decrease
        str_erase(self, index, len-s_len);
    }else if( s_len > len){
        str_fill(self, index, s_len-len);
    }
    char* p = str_at(self, index);
    strncpy(p, s, s_len);
}

static inline str
str_substr(str* self, size_t index, size_t len){
    str substr = str_init("");
    str_reserve(&substr, len+1);
    char* p = str_at(self, index);
    strncpy(substr.c_str, p, len);
    substr.len = len;
    substr.c_str[len] = '\0';
    return substr;
}

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

static inline int
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
            return i + 1 - j;
    }
    return -1;
}