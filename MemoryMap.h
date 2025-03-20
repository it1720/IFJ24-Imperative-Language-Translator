//
// Created by matejbucek on 13.10.24.
//

#ifndef IFJ24_MEMORYMAP_H
#define IFJ24_MEMORYMAP_H

typedef struct MemoryFrame {
  char **variables;
  int size;
  int top;
} MemoryFrame;

typedef struct MemoryFrameStack {
  MemoryFrame **stack;
  int top;
  int size;
} MemoryFrameStack;

typedef struct MemoryMap {
  MemoryFrame *global_frame;
  MemoryFrame *temporary_frame;
  MemoryFrameStack *local_frame;
} MemoryMap;

MemoryMap *memory_map_create();
void memory_map_destroy(MemoryMap **map);
void memory_map_create_frame(MemoryMap *map);
void memory_map_push_frame(MemoryMap *map);
void memory_map_pop_frame(MemoryMap *map);

int memory_frame_variable_exists(MemoryFrame *frame, const char *name);
void memory_frame_def_variable(MemoryFrame *frame, const char *name);

// fqn = (GF|TF|LF)@name
int memory_map_variable_exists(MemoryMap *map, const char *fqn);
int memory_map_def_if_not_exists(MemoryMap *map, const char *fqn);

void memory_map_function_start_reset(MemoryMap *map);

#endif // IFJ24_MEMORYMAP_H
