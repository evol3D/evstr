#ifndef EVSTR_HEADER
#define EVSTR_HEADER

#ifdef EVSTR_DLL
    #if defined(_WINDOWS) || defined(_WIN32)
        #if defined (EVSTR_IMPL)
            #define EVSTR_API __declspec(dllexport)
        #else
            #define EVSTR_API __declspec(dllimport)
        #endif
    #elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
        #if defined (EVSTR_IMPL)
            #define EVSTR_API __attribute__((visibility("default")))
        #else
            #define EVSTR_API
        #endif
    #endif
#else
    #define EVSTR_API
#endif


#ifndef EVSTRING_GROWTH_FACTOR
#define EVSTRING_GROWTH_FACTOR 3 / 2
#endif

#include <stddef.h>
#include <stdarg.h>

typedef char *evstring;

typedef struct evstr_ref {
  evstring data;
  size_t offset;
  size_t len;
} evstr_ref;

EVSTR_API evstring 
evstring_new(
    const char *orig);

EVSTR_API evstring
evstring_clone(
    evstring s);

EVSTR_API void
evstring_free(
    evstring s);

EVSTR_API size_t
evstring_len(
    evstring s);

EVSTR_API int
evstring_setlen(
    evstring *s,
    size_t newlen);

EVSTR_API int
evstring_cmp(
    evstring s1,
    evstring s2);

EVSTR_API int
evstring_pushchar(
    evstring *s,
    char c);

EVSTR_API int
evstring_pushstr(
    evstring *s,
    const char *data);

EVSTR_API int
evstring_pushfmt(
    evstring *s,
    const char *fmt,
    ...);

EVSTR_API int
evstring_pushvfmt(
    evstring *s,
    const char *fmt,
    va_list args);

EVSTR_API evstring
evstring_refclone(
    evstr_ref ref);

EVSTR_API evstr_ref
evstring_ref(
    evstring s);

EVSTR_API evstr_ref
evstring_slice(
    evstring s,
    size_t begin,
    size_t end);

EVSTR_API int
evstring_refpush(
    evstring *s,
    evstr_ref ref);

EVSTR_API size_t
evstring_getspace(
    evstring s);

EVSTR_API int
evstring_addspace(
    evstring *s,
    size_t space);

EVSTR_API void
evstring_clear(
    evstring *s);

EVSTR_API evstring
evstring_newfmt(
    const char *fmt,
    ...);

EVSTR_API evstring
evstring_newvfmt(
    const char *fmt,
    va_list args);

EVSTR_API evstr_ref
evstring_findfirst(
    evstring text,
    const char *query);

EVSTR_API evstring
evstring_replacefirst(
    evstring text,
    const char *query,
    const char *replacement);

EVSTR_API int
evstring_findfirst_ch(
    const evstring text,
    const char c);

EVSTR_API int
evstring_findlast_ch(
    const evstring text,
    const char c);

#if defined(EVSTR_IMPLEMENTATION)

#include <string.h>
#include <assert.h>
#include <stdlib.h>

struct evstring_meta {
    size_t length;
    size_t size;
};

struct evstring_meta *
evstring_getmeta(
        evstring s)
{
    return ((struct evstring_meta *)s) - 1;
}

evstring
evstring_create_impl(
    const char *data,
    size_t len)
{
    size_t size = sizeof(struct evstring_meta) + len + 1;

    void *p = malloc(size);
    assert(p); // Raised if malloc fails

    struct evstring_meta *meta = (struct evstring_meta *)p;
    meta->length = len;
    meta->size = size;

    evstring s = (evstring)(meta + 1);
    if(len > 0) {
        memcpy(s, data, len);
    }
    s[len] = '\0';

    return s;
}

evstring
evstring_newvfmt(
    const char *fmt,
    va_list args)
{
    va_list test;
    va_copy(test, args);
    int len = vsnprintf(NULL, 0, fmt, test);
    evstring res = evstring_create_impl(NULL, 0);
    evstring_addspace(&res, len + 1);
    vsnprintf(res, len + 1, fmt, args);
    evstring_setlen(&res, len);

    va_end(test);

    return res;
}

evstring
evstring_newfmt(
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);

    evstring res = evstring_newvfmt(fmt, ap);

    va_end(ap);

    return res;
}

evstring
evstring_new(
    const char *orig)
{
    size_t origlen = strlen(orig);
    return evstring_create_impl(orig, origlen);
}

evstring
evstring_clone(
    evstring s)
{
    return evstring_create_impl(s, evstring_len(s));
}

void
evstring_free(
    evstring s)
{
    free(evstring_getmeta(s));
}

size_t
evstring_len(
    evstring s)
{
  return evstring_getmeta(s)->length;
}

int
evstring_setsize(
    evstring *s, 
    size_t newsize)
{
    struct evstring_meta *meta = evstring_getmeta(*s);
    if(meta->size == newsize) {
        return 0;
    }

    void *buf = (void*)meta;
    void *tmp = realloc(buf, sizeof(struct evstring_meta) + newsize);

    if (!tmp) {
        return 1;
    }

    if(buf != tmp) { // Reallocation caused memory to be moved
        buf = tmp;
        meta = (struct evstring_meta *)buf;
        *s = (char*)buf + sizeof(struct evstring_meta);
    }

    meta->size = newsize;
    return 0;
}

int
evstring_grow(
    evstring *s)
{
    return evstring_setsize(s, evstring_getmeta(*s)->size * EVSTRING_GROWTH_FACTOR);
}

int
evstring_setlen(
    evstring *s,
    size_t newlen)
{
    struct evstring_meta *meta = evstring_getmeta(*s);
    if(newlen == meta->length) {
        return 0;
    }

    while(newlen > meta->size) {
        if(evstring_grow(s)) {
            return 1;
        }
        meta = evstring_getmeta(*s);
    }
    meta->length = newlen;

    return 0;
}

void
evstring_clear(
    evstring *s)
{
    evstring_setlen(s, 0);
}

int
evstring_cmp(
    evstring s1,
    evstring s2)
{
    size_t len1 = evstring_len(s1);
    size_t len2 = evstring_len(s2);
    if(len1 != len2) {
        return 1;
    }
    return memcmp(s1, s2, len1);
}


int
evstring_push(
    evstring *s,
    size_t sz,
    const char *data)
{
    struct evstring_meta *meta = evstring_getmeta(*s);

    // TODO Find a more efficient approach?
    while(meta->size <= sizeof(struct evstring_meta) + meta->length + sz) { // `<=` because of the null terminator
        if(evstring_grow(s) != 0) {
            return 1; // Failed
        }
        meta = evstring_getmeta(*s);
    }

    memcpy((*s) + meta->length, data, sz);
    meta->length += sz;

    (*s)[meta->length] = '\0';
    return 0;
}

int
evstring_pushchar(
    evstring *s,
    char c)
{
    return evstring_push(s, 1, &c);
}

int
evstring_pushstr(
    evstring *s,
    const char *data)
{
    return evstring_push(s,strlen(data),data);
}

evstring
evstring_cat(
    evstring s1,
    evstring s2)
{
    evstring ns = evstring_clone(s1);
    evstring_push(&ns, evstring_len(s2), s2);
    return ns;
}

int
evstring_refpush(
    evstring *s,
    evstr_ref ref)
{
    return evstring_push(s,ref.len,ref.data + ref.offset);
}

evstring
evstring_refclone(
    evstr_ref ref)
{
    return evstring_create_impl(ref.data + ref.offset, ref.len);
}

evstr_ref
evstring_ref(
    evstring s)
{
    return (evstr_ref) {
        .data = s, 
        .offset = 0, 
        .len = evstring_len(s)};
}

evstr_ref
evstring_slice(
    evstring s,
    size_t begin,
    size_t end)
{
    return (evstr_ref) {
        .data = s,
        .offset = begin, 
        .len = end - begin
    };
}

size_t
evstring_getspace(
    evstring s)
{
    struct evstring_meta *meta = evstring_getmeta(s);
    return meta->size - meta->length;
}

EVSTR_API int
evstring_addspace(
    evstring *s,
    size_t space)
{
    return evstring_setsize(s, evstring_getmeta(*s)->size + space);
}

EVSTR_API evstr_ref
evstring_findfirst(
    evstring text,
    const char *query)
{
    size_t query_len = strlen(query);
    size_t text_len = evstring_len(text);
    size_t found_progress = 0;

    evstr_ref result = {
        .data = text,
        .len = 0,
        .offset = ~0ull
    };

    for(size_t i = 0; i < text_len; i++) {
        if(text[i] == query[found_progress]) {
            found_progress++;
        }
        if(found_progress == query_len) {
            result.offset = (i+1) - query_len;
            result.len = query_len;
            break;
        }
    }
    return result;
}

EVSTR_API evstring
evstring_replacefirst(
    evstring text,
    const char *query,
    const char *replacement)
{
    evstring result = NULL;

    evstr_ref query_slice = evstring_findfirst(text, query);

    // If the query doesn't actually exist, then we're returning a clone of
    // the original string.
    if(query_slice.len == 0) {
        result = evstring_clone(text);
    } else {
        result=evstring_create_impl(NULL,0);

        // If the query doesn't match at the beginning of the string,
        // then we need to copy the data before it first.
        if(query_slice.offset != 0) {
            evstring_refpush(&result, evstring_slice(text, 0, query_slice.offset));
        }

        // Then, we simply push the replacement
        evstring_pushstr(&result, replacement);

        // Followed by the rest of the string
        evstring_refpush(&result,
            evstring_slice(text, query_slice.offset + query_slice.len, evstring_len(text)));
    }
    return result;
}

EVSTR_API int
evstring_pushfmt(
    evstring *s,
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);
    evstring_pushvfmt(s, fmt, ap);
    va_end(ap);
}

EVSTR_API int
evstring_pushvfmt(
    evstring *s,
    const char *fmt,
    va_list args)
{
    va_list test;
    va_copy(test, args);
    int fmt_len = vsnprintf(NULL, 0, fmt, test);
    size_t old_len = evstring_len(*s);
    evstring_setlen(s, old_len + fmt_len);
    vsnprintf((*s) + old_len, fmt_len, fmt, args);
    va_end(test);
}

EVSTR_API int
evstring_findfirst_ch(
    const evstring text,
    const char c)
{
    struct evstring_meta *meta = evstring_getmeta(text);
    for(int i = 0; i < meta->length; i++) {
        if(text[i] == c) {
            return i;
        }
    }
    return -1;
}

EVSTR_API int
evstring_findlast_ch(
    const evstring text,
    const char c)
{
    struct evstring_meta *meta = evstring_getmeta(text);
    int i;
    for(i = meta->length - 1; i >= meta->length; i--) {
        if(text[i] == c) {
            break;
        }
    }
    return i; 
}

#endif

#endif
