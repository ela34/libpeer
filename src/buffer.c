#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "buffer.h"

#if BUFFER_USE_MUTEX
# include <pthread.h>
# define PEER_MUTEX_INIT(X)    static pthread_mutex_t X = PTHREAD_MUTEX_INITIALIZER
# define PEER_MUTEX_LOCK(X)    pthread_mutex_lock(X)
# define PEER_MUTEX_UNLOCK(X)  pthread_mutex_unlock(X)
#else
# define PEER_MUTEX_INIT(X)
# define PEER_MUTEX_LOCK(X)
# define PEER_MUTEX_UNLOCK(X)
#endif

PEER_MUTEX_INIT(g_buffer_mutex);

Buffer* buffer_new(int size) {

  Buffer *rb;
  rb = (Buffer*)calloc(1, sizeof(Buffer));

  rb->data = (uint8_t*)calloc(1, size);
  rb->size = size;
  rb->head = 0;
  rb->tail = 0;

  return rb;
}

void buffer_clear(Buffer *rb) {
  PEER_MUTEX_LOCK(&g_buffer_mutex);
  if (rb) {
    rb->head = 0;
    rb->tail = 0;
  }
  PEER_MUTEX_UNLOCK(&g_buffer_mutex);
}

void buffer_free(Buffer *rb) {
  PEER_MUTEX_LOCK(&g_buffer_mutex);
  if (rb) {
    free(rb->data);
    rb->data = NULL;
    free(rb);  // Now properly freeing the buffer itself
  }
  PEER_MUTEX_UNLOCK(&g_buffer_mutex);
}

int buffer_push_tail(Buffer *rb, const uint8_t *data, int size) {
  PEER_MUTEX_LOCK(&g_buffer_mutex);

  int free_space = (rb->size + rb->head - rb->tail - 1) % rb->size;
  int align_size = ALIGN32(size + 4);

  if (align_size > free_space) {
    LOGE("no enough space");
    PEER_MUTEX_UNLOCK(&g_buffer_mutex);
    return -1;
  }

  int tail_end = (rb->tail + align_size) % rb->size;

  if (tail_end < rb->tail) {
    // Handle wrap-around
    if (rb->head < align_size) {
      LOGE("no enough space");
      PEER_MUTEX_UNLOCK(&g_buffer_mutex);
      return -1;
    }

    int *p = (int*)(rb->data + rb->tail);
    *p = size;
    memcpy(rb->data + rb->tail + 4, data, rb->size - rb->tail - 4);
    memcpy(rb->data, data + (rb->size - rb->tail - 4), tail_end);
  } else {
    int *p = (int*)(rb->data + rb->tail);
    *p = size;
    memcpy(rb->data + rb->tail + 4, data, size);
  }

  rb->tail = tail_end;

  PEER_MUTEX_UNLOCK(&g_buffer_mutex);
  return size;
}

uint8_t* buffer_peak_head(Buffer *rb, int *size) {
  PEER_MUTEX_LOCK(&g_buffer_mutex);
  if (!rb || rb->head == rb->tail) {
    (*size) = 0;
    PEER_MUTEX_UNLOCK(&g_buffer_mutex);
    return NULL;
  }

  *size = *((int*)(rb->data + rb->head));

  int align_size = ALIGN32(*size + 4);
  int head_end = (rb->head + align_size) % rb->size;

  PEER_MUTEX_UNLOCK(&g_buffer_mutex);

  if (head_end < rb->head) {
    return rb->data;  // Returns pointer to start of buffer in case of wrap-around
  } else {
    return rb->data + (rb->head + 4);
  }
}

void buffer_pop_head(Buffer *rb) {
  PEER_MUTEX_LOCK(&g_buffer_mutex);

  if (!rb || rb->head == rb->tail) {
    PEER_MUTEX_UNLOCK(&g_buffer_mutex);
    return;
  }

  int *size = (int*)(rb->data + rb->head);
  int align_size = ALIGN32(*size + 4);
  rb->head = (rb->head + align_size) % rb->size;

  PEER_MUTEX_UNLOCK(&g_buffer_mutex);
}
