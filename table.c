#include <table.h>

#define T Table_T
#define CHECKNULL(r, c, e) \
  if ((r = c) == NULL) {   \
    perror(e);             \
    exit(errno);           \
  }

struct T {
  int length;  // number of pairs
  int size;
  int (*cmp)(const void* x, const void* y);
  unsigned (*hash)(const void* key);
  struct pair {
    struct pair* link;
    const void* key;
    void* value;
  } * *buckets;
};
// static le rende private
static int cmpatom(const void* x, const void* y) {
  return x != y;
}

static unsigned hashatom(const void* key) {
  return 2;  //(unsigned long)key >> 2;
}

T Table_new(int hint,
            int cmp(const void* x, const void* y),
            unsigned hash(const void* key)) {
  T table;
  int i;
  static int primes[] = {503, 509, 1021, 2053, 4093, 8191, 16381, INT_MAX};
  assert(hint >= 0);
  for (i = 1; primes[i] < hint; i++)
    ;

  CHECKNULL(table,
            malloc(sizeof(*table) + primes[i - 1] * sizeof(table->buckets[0])),
            "table malloc");

  table->size = primes[i - 1];
  table->cmp = cmp ? cmp : cmpatom;
  table->hash = hash ? hash : hashatom;
  table->buckets = (struct pair**)(table + 1);
  for (i = 0; i < table->size; i++) {
    table->buckets[i] = NULL;
  }
  table->length = 0;

  return table;
}

void* Table_get(T table, const void* key) {
  int i;
  struct pair* p;
  assert(table);
  assert(key);

  i = (*table->hash)(key) % table->size;
  for (p = table->buckets[i]; p; p = p->link) {
    if ((*table->cmp)(key, p->key) == 0)
      break;
  }
  return p ? p->value : NULL;
}

void* Table_put(T table, const void* key, void* value) {
  int i;
  struct pair* p;
  void* prev;

  assert(table);
  assert(key);
  i = (*table->hash)(key) % table->size;
  for (p = table->buckets[i]; p; p = p->link) {
    if ((*table->cmp)(key, p->key) == 0)
      break;
  }
  if (p == NULL) {
    CHECKNULL(p, malloc((long)sizeof *(p)), "entry malloc");
    // CHECKNULL(p->key, malloc(sizeof(p->key)), "entry key malloc");
    p->key = key;
    p->link = table->buckets[i];
    table->buckets[i] = p;
    table->length++;
    prev = NULL;
  } else {
    prev = p->value;
  }
  p->value = value;
  return prev;
}

int Table_length(T table) {
  assert(table);
  return table->length;
}

void* Table_remove(T table, const void* key) {
  int i;
  struct pair** pp;

  assert(table);
  assert(key);

  i = (*table->hash)(key) % table->size;
  for (pp = &table->buckets[i]; *pp; pp = &(*pp)->link) {
    if ((*table->cmp)(key, (*pp)->key) == 0) {
      struct pair* p = *pp;
      void* value = p->value;
      *pp = p->link;
      free(p);
      table->length--;
      return value;
    }
  }
  return NULL;
}

void Table_free(T* table) {
  assert(table && *table);
  if ((*table)->length > 0) {
    int i;
    struct pair *p, *q;
    for (i = 0; i < (*table)->size; i++) {
      for (p = (*table)->buckets[i]; p; p = q) {
        q = p->link;
        free(p);
      }
    }
  }
  free(*table);
}

void TableArray(T table) {
  struct pair* p;
  int i;
  for (i = 0; i < table->size; i++)
    for (p = table->buckets[i]; p; p = p->link) {
      printf("key=%s\n", p->key);
      printf("value=%d\n", p->value);
    }
}