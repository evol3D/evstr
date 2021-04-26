#ifndef EVSTR_HEADER
#define EVSTR_HEADER

#ifndef EVSTRING_GROWTH_FACTOR
#define EVSTRING_GROWTH_FACTOR 3 / 2
#endif

typedef char *evstring;

typedef struct evstr_ref {
  size_t len;
  const char *data;
} evstr_ref;

evstring 
evstring_new(
    const char *orig);

evstring
evstring_clone(
    evstring s);

void
evstring_free(
    evstring s);

size_t
evstring_len(
    evstring s);

void
evstring_setlen(
    evstring s,
    size_t newlen);

int
evstring_cmp(
    evstring s1,
    evstring s2);

int
evstring_pushchar(
    evstring *s,
    char c);

int
evstring_pushstr(
    evstring *s,
    const char *data);

evstring
evstring_refclone(
    evstr_ref ref);

evstr_ref
evstring_ref(
    evstring s);

evstr_ref
evstring_slice(
    evstring s,
    size_t begin,
    size_t end);

int
evstring_refpush(
    evstring *s,
    evstr_ref ref);

#if defined(EVSTR_IMPL)

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
    memcpy(s, data, len);
    s[len] = '\0';

    return s;
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

void
evstring_setlen(
    evstring s,
    size_t newlen)
{
    evstring_getmeta(s)->length = 0;
}

void
evstring_clear(
    evstring *s)
{
    evstring_setlen(*s, 0);
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
        *s = buf + sizeof(struct evstring_meta);
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
    return evstring_push(s,ref.len,ref.data);
}

evstring
evstring_refclone(
    evstr_ref ref)
{
    return evstring_create_impl(ref.data, ref.len);
}

evstr_ref
evstring_ref(
    evstring s)
{
    return (evstr_ref) {.data = s, .len = evstring_len(s)};
}

evstr_ref
evstring_slice(
    evstring s,
    size_t begin,
    size_t end)
{
    return (evstr_ref) {.data = s + begin, .len = end - begin};
}

#endif

#endif
