#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  LOG_LEVEL_DEBUG = 1,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_PANIC,
};

void vm_logger_set_log_level(uint8_t log_level);
void vm_logger_assign_log_level();
uint8_t *vm_logger_get_log_level();

uint16_t vm_logger_initialized = 0;

uint8_t *vm_logger_get_log_level() {
  static uint8_t g_log_level = 0;
  if (vm_logger_initialized == 0) {
    vm_logger_initialized = 1;
    vm_logger_assign_log_level();
  }

  return &g_log_level;
}

void vm_logger_set_log_level(uint8_t log_level) {
  uint8_t *g_log_level = vm_logger_get_log_level();
  *g_log_level = log_level;
}

void vm_logger_assign_log_level() {
  char *log_level_env = getenv("VM_LOG_LEVEL");
  uint8_t default_log_level = LOG_LEVEL_WARN;
  if (log_level_env == NULL) {
    vm_logger_set_log_level(default_log_level);
    return;
  }

  if (strcmp(log_level_env, "DEBUG") == 0) {
    vm_logger_set_log_level(LOG_LEVEL_DEBUG);
    return;
  }

  if (strcmp(log_level_env, "INFO") == 0) {
    vm_logger_set_log_level(LOG_LEVEL_INFO);
    return;
  }

  if (strcmp(log_level_env, "WARN") == 0) {
    vm_logger_set_log_level(LOG_LEVEL_WARN);
    return;
  }

  if (strcmp(log_level_env, "PANIC") == 0) {
    vm_logger_set_log_level(LOG_LEVEL_PANIC);
    return;
  }
}

// logger functions
void vm_logger_dev(char *msg, ...) {
  if (*(vm_logger_get_log_level()) > LOG_LEVEL_DEBUG) {
    return;
  }

  va_list args;
  va_start(args, msg);
  printf("[VM DEV] ");
  vprintf(msg, args);
  va_end(args);
}

void vm_logger_info(char *msg, ...) {
  if (*vm_logger_get_log_level() > LOG_LEVEL_INFO) {
    return;
  }

  va_list args;
  va_start(args, msg);
  printf("[VM INFO] ");
  vprintf(msg, args);
  va_end(args);
}

void vm_logger_warn(char *msg, ...) {
  if (*vm_logger_get_log_level() > LOG_LEVEL_WARN) {
    return;
  }

  va_list args;
  va_start(args, msg);
  printf("[VM WARN] ");
  vprintf(msg, args);
  va_end(args);
}

void vm_logger_panic(char *msg, ...) {
  if (*vm_logger_get_log_level() > LOG_LEVEL_PANIC) {
    return;
  }

  va_list args;
  va_start(args, msg);
  printf("[VM PANIC] ");
  vprintf(msg, args);
  va_end(args);
}
