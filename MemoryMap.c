//
// Created by matejbucek on 13.10.24.
//

#include "MemoryMap.h"
#include "error.h"
#include <string.h>

MemoryFrame *create_frame() {
  MemoryFrame *frame = malloc(sizeof(MemoryFrame));
  if (!frame) {
    error(99, 0, "Malloc error");
  }

  frame->size = 200;
  frame->top = -1;
  frame->variables = malloc(sizeof(char *) * 200);

  return frame;
}

void destroy_frame(MemoryFrame *frame) {
  if (!frame)
    return;

  free(frame);
}

int memory_frame_variable_exists(MemoryFrame *frame, const char *name) {
  if (!frame)
    return 0;

  for (int i = 0; i <= frame->top; i++) {
    if (strcmp(frame->variables[i], name) == 0) {
      return 1;
    }
  }

  return 0;
}

void memory_frame_def_variable(MemoryFrame *frame, const char *name) {
  if (!frame) {
    return;
  }

  if (frame->top == frame->size - 1) {
    error(99, 0, "Memory frame is full");
  }

  frame->top++;
  frame->variables[frame->top] = strdup(name);
}

MemoryFrameStack *create_frame_stack() {
  MemoryFrameStack *stack = malloc(sizeof(MemoryFrameStack));
  if (!stack) {
    error(99, 0, "Malloc error");
  }

  stack->size = 200;
  stack->top = -1;
  stack->stack = malloc(sizeof(MemoryFrameStack *) * 200);
  if (!stack->stack) {
    error(99, 0, "Malloc error");
  }

  return stack;
}

void destroy_frame_stack(MemoryFrameStack **stack) {
  if (!stack || !*stack)
    return;

  for (int i = 0; i < (*stack)->size; i++) {
    destroy_frame((*stack)->stack[i]);
  }

  free(*stack);
  *stack = NULL;
}

MemoryMap *memory_map_create() {
  MemoryMap *map = malloc(sizeof(MemoryMap));
  if (!map) {
    error(99, 0, "Malloc error");
  }

  map->global_frame = create_frame();
  map->temporary_frame = NULL;
  map->local_frame = create_frame_stack();
  return map;
}

void memory_map_destroy(MemoryMap **map) {}

void memory_map_create_frame(MemoryMap *map) {
  if (map->temporary_frame != NULL) {
    destroy_frame(map->temporary_frame);
  }

  map->temporary_frame = create_frame();
}

void memory_map_push_frame(MemoryMap *map) {
  if (map->local_frame->top == map->local_frame->size - 1) {
    error(99, 0, "Memory frame stack is full");
  }

  map->local_frame->top++;
  map->local_frame->stack[map->local_frame->top] = map->temporary_frame;
  map->temporary_frame = NULL;
}

void memory_map_pop_frame(MemoryMap *map) {
  if (map->local_frame->top == -1) {
    error(99, 0, "Memory frame stack is empty");
  }

  destroy_frame(map->temporary_frame);
  map->temporary_frame = map->local_frame->stack[map->local_frame->top];
  map->local_frame->top--;
}

// fqn = (GF|TF|LF)@name
int memory_map_variable_exists(MemoryMap *map, const char *fqn) {
  if (strlen(fqn) < 2) {
    return 0;
  }

  if (fqn[0] == 'G' && fqn[1] == 'F') {
    return memory_frame_variable_exists(map->global_frame, fqn + 3);
  } else if (fqn[0] == 'T' && fqn[1] == 'F') {
    return memory_frame_variable_exists(map->temporary_frame, fqn + 3);
  } else if (fqn[0] == 'L' && fqn[1] == 'F') {
    if (map->local_frame->top == -1) {
      return 0;
    }
    return memory_frame_variable_exists(
        map->local_frame->stack[map->local_frame->top], fqn + 3);
  } else {
    return 0;
  }
}

int memory_map_def_if_not_exists(MemoryMap *map, const char *fqn) {
  if (memory_map_variable_exists(map, fqn)) {
    fprintf(stderr, "Variable %s already exists\n", fqn);
    return 0;
  }

  if (strlen(fqn) < 2) {
    fprintf(stderr, "Variable %s is too short\n", fqn);
    return 0;
  }

  if (fqn[0] == 'G' && fqn[1] == 'F') {
    fprintf(stderr, "Defining variable %s in global frame\n", fqn);
    memory_frame_def_variable(map->global_frame, fqn + 3);
  } else if (fqn[0] == 'T' && fqn[1] == 'F') {
    fprintf(stderr, "Defining variable %s in temporary frame\n", fqn);
    memory_frame_def_variable(map->temporary_frame, fqn + 3);
  } else if (fqn[0] == 'L' && fqn[1] == 'F') {
    if (map->local_frame->top == -1) {
      return 0;
    }
    fprintf(stderr, "Defining variable %s in local frame\n", fqn);
    memory_frame_def_variable(map->local_frame->stack[map->local_frame->top],
                              fqn + 3);
  } else {
    return 0;
  }

  return 1;
}

void memory_map_function_start_reset(MemoryMap *map) {
  map->local_frame = create_frame_stack();
  memory_map_create_frame(map);
  memory_map_push_frame(map);
}