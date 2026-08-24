#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum Sr2apResult {
  SR2AP_RESULT_OK = 0,
  SR2AP_RESULT_INVALID_ARGUMENT = 1,
  SR2AP_RESULT_NOT_INITIALIZED = 2,
  SR2AP_RESULT_IO_ERROR = 3,
  SR2AP_RESULT_INTERNAL_ERROR = 4,
} Sr2apResult;

typedef uint32_t Sr2apLogLevel;

#define SR2AP_LOG_LEVEL_TRACE 0

#define SR2AP_LOG_LEVEL_DEBUG 1

#define SR2AP_LOG_LEVEL_INFO 2

#define SR2AP_LOG_LEVEL_WARNING 3

#define SR2AP_LOG_LEVEL_ERROR 4

#define SR2AP_LOG_LEVEL_CRITICAL 5

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Initializes the logger.
 *
 * `directory` is borrowed UTF-16 data and is only read for this call.
 *
 */
enum Sr2apResult sr2ap_log_initialize(const uint16_t *directory,
                                      size_t directory_len,
                                      uint8_t debug_enabled);

/**
 * Writes one record to the logger.
 */
enum Sr2apResult sr2ap_log_write(Sr2apLogLevel level,
                                 const uint8_t *subsystem,
                                 size_t subsystem_len,
                                 const uint8_t *message,
                                 size_t message_len);

enum Sr2apResult sr2ap_log_flush(void);

void sr2ap_log_shutdown(void);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
