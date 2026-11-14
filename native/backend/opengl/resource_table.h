#ifndef LUNARRENDER_OPENGL_RESOURCE_TABLE_H
#define LUNARRENDER_OPENGL_RESOURCE_TABLE_H

/*
 * Fixed-capacity resource table for the OpenGL backend.
 *
 * The backend keeps render resources in fixed slot arrays. Two operations used
 * to be linear in the slot capacity:
 *
 *   - resolving a handle key to a slot (scan of all slots), and
 *   - walking the slots to find every live resource during a frame.
 *
 * Both are now independent of the capacity: `lr_table_find` is O(1) expected
 * through an open-addressing hash table, and frames walk `active[]`, which is
 * a dense list holding exactly the live slots. The table owns slot
 * allocation (a free list), so uploads stay O(1) as well.
 *
 * Deletion uses tombstones and rebuilds the index once they pile up, which
 * keeps the probe sequences short without needing backward-shift deletion.
 */

#include <stdbool.h>
#include <string.h>

#define LR_TABLE_MAX_SLOTS 256
#define LR_TABLE_MAX_BUCKETS 512

typedef struct {
  int capacity;
  int bucket_mask;
  int buckets[LR_TABLE_MAX_BUCKETS]; /* key hash -> slot + 1, 0 empty, -1 tombstone */
  int slot_key[LR_TABLE_MAX_SLOTS];  /* slot -> key, -1 for a free slot */
  int active[LR_TABLE_MAX_SLOTS];    /* dense list of live slots */
  int active_pos[LR_TABLE_MAX_SLOTS];/* slot -> index in active[] */
  int active_count;
  int free_slots[LR_TABLE_MAX_SLOTS];
  int free_count;
  int tombstone_count;
} LunarrenderResourceTable;

static inline unsigned int lr_table_hash(int key) {
  /* Fibonacci hashing: maps arbitrary handle keys over the bucket array. */
  unsigned int hash = (unsigned int)key * 2654435761u;
  return hash >> 8;
}

static inline int lr_table_find(const LunarrenderResourceTable *table, int key) {
  if (table->capacity <= 0) {
    return -1;
  }
  int index = (int)(lr_table_hash(key) & (unsigned int)table->bucket_mask);
  for (int probe = 0; probe <= table->bucket_mask; probe++) {
    int cell = table->buckets[index];
    if (cell == 0) {
      return -1;
    }
    if (cell > 0 && table->slot_key[cell - 1] == key) {
      return cell - 1;
    }
    index = (index + 1) & table->bucket_mask;
  }
  return -1;
}

static inline void lr_table_reindex(LunarrenderResourceTable *table) {
  memset(table->buckets, 0, sizeof(table->buckets));
  table->tombstone_count = 0;
  for (int position = 0; position < table->active_count; position++) {
    int slot = table->active[position];
    int index =
        (int)(lr_table_hash(table->slot_key[slot]) & (unsigned int)table->bucket_mask);
    while (table->buckets[index] != 0) {
      index = (index + 1) & table->bucket_mask;
    }
    table->buckets[index] = slot + 1;
  }
}

static inline void lr_table_init(LunarrenderResourceTable *table, int capacity) {
  if (capacity > LR_TABLE_MAX_SLOTS) {
    capacity = LR_TABLE_MAX_SLOTS;
  }
  table->capacity = capacity;
  table->bucket_mask = LR_TABLE_MAX_BUCKETS - 1;
  memset(table->buckets, 0, sizeof(table->buckets));
  for (int slot = 0; slot < LR_TABLE_MAX_SLOTS; slot++) {
    table->slot_key[slot] = -1;
    table->active_pos[slot] = -1;
    table->free_slots[slot] = capacity - 1 - slot;
  }
  table->active_count = 0;
  table->free_count = capacity;
  table->tombstone_count = 0;
}

static inline int lr_table_acquire(LunarrenderResourceTable *table, int key) {
  int existing = lr_table_find(table, key);
  if (existing >= 0) {
    return existing;
  }
  if (table->free_count <= 0) {
    return -1;
  }
  int slot = table->free_slots[--table->free_count];
  int index =
      (int)(lr_table_hash(key) & (unsigned int)table->bucket_mask);
  int target = -1;
  for (int probe = 0; probe <= table->bucket_mask; probe++) {
    int cell = table->buckets[index];
    if (cell == 0) {
      if (target < 0) {
        target = index;
      }
      break;
    }
    if (cell < 0 && target < 0) {
      target = index;
    }
    index = (index + 1) & table->bucket_mask;
  }
  if (target < 0) {
    table->free_slots[table->free_count++] = slot;
    return -1;
  }
  if (table->buckets[target] < 0) {
    table->tombstone_count--;
  }
  table->buckets[target] = slot + 1;
  table->slot_key[slot] = key;
  table->active_pos[slot] = table->active_count;
  table->active[table->active_count++] = slot;
  return slot;
}

static inline void lr_table_remove(LunarrenderResourceTable *table, int slot) {
  if (slot < 0 || slot >= table->capacity || table->slot_key[slot] < 0) {
    return;
  }
  int key = table->slot_key[slot];
  int index = (int)(lr_table_hash(key) & (unsigned int)table->bucket_mask);
  for (int probe = 0; probe <= table->bucket_mask; probe++) {
    int cell = table->buckets[index];
    if (cell == 0) {
      break;
    }
    if (cell == slot + 1) {
      table->buckets[index] = -1;
      table->tombstone_count++;
      break;
    }
    index = (index + 1) & table->bucket_mask;
  }
  table->slot_key[slot] = -1;
  int position = table->active_pos[slot];
  int last_slot = table->active[table->active_count - 1];
  table->active[position] = last_slot;
  table->active_pos[last_slot] = position;
  table->active_pos[slot] = -1;
  table->active_count--;
  table->free_slots[table->free_count++] = slot;
  if (table->tombstone_count > 32) {
    lr_table_reindex(table);
  }
}

#endif /* LUNARRENDER_OPENGL_RESOURCE_TABLE_H */
