/*
 * Standalone benchmark and self-check for backend/opengl/resource_table.h.
 *
 * The OpenGL backend cannot be profiled without a window and a GL context, but
 * the hot path that this repository changed is pure bookkeeping: resolving a
 * handle key to a slot, and walking the live slots once per frame. Both used
 * to be linear in the slot capacity. This tool measures the alternatives
 * directly and verifies the table against a reference scan, so the numbers in
 * docs/performance.md can be reproduced without a GPU.
 *
 * Build (MSVC developer prompt):
 *   cl /std:c11 /O2 /Fe:resource_table_bench.exe tools\resource_table_bench.c
 * Build (gcc/clang):
 *   cc -O2 -o resource_table_bench tools/resource_table_bench.c
 * Run:
 *   resource_table_bench            # correctness check + timings
 *   resource_table_bench --check    # correctness only
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "../backend/opengl/resource_table.h"

#define SLOTS 256
#define LOOPS 200000
#define FRAMES 20000

static uint64_t now_ns(void) {
#ifdef _WIN32
  static LARGE_INTEGER frequency;
  static bool initialized = false;
  if (!initialized) {
    QueryPerformanceFrequency(&frequency);
    initialized = true;
  }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (uint64_t)(counter.QuadPart * 1000000000ull / frequency.QuadPart);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

/* The implementation the backend used before the table: a scan of every slot
 * for a key, and a scan of every slot for iteration. */
typedef struct {
  int active[SLOTS];
  int key[SLOTS];
  long comparisons;
} ScanTable;

static void scan_init(ScanTable *table, int count) {
  memset(table, 0, sizeof(*table));
  for (int i = 0; i < SLOTS; i++) {
    table->key[i] = -1;
  }
  for (int i = 0; i < count; i++) {
    table->active[i] = 1;
    table->key[i] = i * 7 + 3;
  }
}

static int scan_find(ScanTable *table, int needle) {
  for (int i = 0; i < SLOTS; i++) {
    table->comparisons++;
    if (table->active[i] && table->key[i] == needle) {
      return i;
    }
  }
  return -1;
}

static int scan_iterate(ScanTable *table) {
  int live = 0;
  for (int i = 0; i < SLOTS; i++) {
    table->comparisons++;
    if (!table->active[i]) {
      continue;
    }
    live++;
  }
  return live;
}

static int check_table(void) {
  LunarrenderResourceTable table;
  lr_table_init(&table, SLOTS);
  int live = 0;
  unsigned int seed = 12345u;
  for (int step = 0; step < 20000; step++) {
    seed = seed * 1103515245u + 12345u;
    int key = (int)((seed >> 16) % 400);
    int slot = lr_table_find(&table, key);
    if (slot < 0) {
      int acquired = lr_table_acquire(&table, key);
      if (acquired < 0) {
        if (live != SLOTS) {
          fprintf(stderr, "check: acquire failed with %d live slots\n", live);
          return 1;
        }
        continue;
      }
      if (lr_table_find(&table, key) != acquired) {
        fprintf(stderr, "check: acquired slot is not findable\n");
        return 1;
      }
      live++;
    } else if (((seed >> 8) & 3) == 0) {
      lr_table_remove(&table, slot);
      live--;
      if (lr_table_find(&table, key) >= 0) {
        fprintf(stderr, "check: removed key is still findable\n");
        return 1;
      }
    }
  }
  int active = 0;
  for (int position = 0; position < table.active_count; position++) {
    int slot = table.active[position];
    if (table.slot_key[slot] < 0) {
      fprintf(stderr, "check: active list holds a free slot\n");
      return 1;
    }
    if (lr_table_find(&table, table.slot_key[slot]) != slot) {
      fprintf(stderr, "check: active slot does not resolve to itself\n");
      return 1;
    }
    if (table.active_pos[slot] != position) {
      fprintf(stderr, "check: active_pos is stale\n");
      return 1;
    }
    active++;
  }
  if (active != live) {
    fprintf(stderr, "check: active count %d != live %d\n", active, live);
    return 1;
  }
  if (table.active_count + table.free_count != SLOTS) {
    fprintf(stderr, "check: slot accounting is inconsistent\n");
    return 1;
  }
  printf("check: ok (%d live slots)\n", live);
  return 0;
}

static void report(
    const char *name,
    uint64_t elapsed_ns,
    long work,
    long comparisons) {
  double per_op = (double)elapsed_ns / (double)work;
  printf(
      "%-24s %9.2f ns/op   %10ld comparisons   %6.2f ns/comparison\n",
      name,
      per_op,
      comparisons,
      comparisons > 0 ? (double)elapsed_ns / (double)comparisons : 0.0);
}

static int bench(void) {
  ScanTable scan;
  scan_init(&scan, SLOTS);
  LunarrenderResourceTable table;
  lr_table_init(&table, SLOTS);
  for (int i = 0; i < SLOTS; i++) {
    lr_table_acquire(&table, i * 7 + 3);
  }

  volatile int sink = 0;

  uint64_t start = now_ns();
  for (int i = 0; i < LOOPS; i++) {
    sink += scan_find(&scan, ((i * 31) % SLOTS) * 7 + 3);
  }
  uint64_t scan_find_ns = now_ns() - start;
  long scan_find_comparisons = scan.comparisons;

  start = now_ns();
  for (int i = 0; i < LOOPS; i++) {
    sink += lr_table_find(&table, ((i * 31) % SLOTS) * 7 + 3);
  }
  uint64_t table_find_ns = now_ns() - start;

  scan.comparisons = 0;
  start = now_ns();
  for (int i = 0; i < LOOPS; i++) {
    sink += scan_iterate(&scan);
  }
  uint64_t scan_iter_ns = now_ns() - start;
  long scan_iter_comparisons = scan.comparisons;

  start = now_ns();
  for (int i = 0; i < LOOPS; i++) {
    sink += table.active_count;
  }
  uint64_t table_iter_ns = now_ns() - start;

  printf("slots=%d loops=%d\n\n", SLOTS, LOOPS);
  report("scan find (hit)", scan_find_ns, LOOPS, scan_find_comparisons);
  report("table find (hit)", table_find_ns, LOOPS, 0);
  report("scan iterate", scan_iter_ns, LOOPS, scan_iter_comparisons);
  report("table iterate", table_iter_ns, LOOPS, 0);

  /* One frame with SLOTS meshes. The old backend scanned all slots, then
   * scanned for the material and the texture of every mesh. */
  scan.comparisons = 0;
  start = now_ns();
  for (int frame = 0; frame < FRAMES; frame++) {
    int drawn = 0;
    for (int i = 0; i < SLOTS; i++) {
      scan.comparisons++;
      if (!scan.active[i]) {
        continue;
      }
      scan_find(&scan, i * 7 + 3);
      scan_find(&scan, i * 7 + 3);
      drawn++;
    }
    sink += drawn;
  }
  uint64_t old_frame_ns = now_ns() - start;
  long old_frame_comparisons = scan.comparisons;

  start = now_ns();
  for (int frame = 0; frame < FRAMES; frame++) {
    int drawn = 0;
    for (int position = 0; position < table.active_count; position++) {
      int mesh_slot = table.active[position];
      int material_slot = lr_table_find(&table, table.slot_key[mesh_slot]);
      int texture_slot = lr_table_find(&table, material_slot);
      sink += material_slot + texture_slot;
      drawn++;
    }
    sink += drawn;
  }
  uint64_t new_frame_ns = now_ns() - start;

  printf("\nframe with %d meshes x %d frames\n", SLOTS, FRAMES);
  report("scan-based frame", old_frame_ns, FRAMES, old_frame_comparisons);
  report("table-based frame", new_frame_ns, FRAMES, 0);
  printf(
      "speedup: %.1fx (CPU-side submit only; GPU cost is unchanged)\n",
      (double)old_frame_ns / (double)new_frame_ns);
  printf("(sink=%d)\n", sink);
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--check") == 0) {
    return check_table();
  }
  int failed = check_table();
  if (failed) {
    return failed;
  }
  printf("\n");
  return bench();
}
