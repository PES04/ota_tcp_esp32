#ifndef SYS_FEEDBACK_H
#define SYS_FEEDBACK_H

#include <stdint.h>
#include "types.h"

typedef enum {
    SYS_FEEDBACK_MODE_UPDATE,
    SYS_FEEDBACK_MODE_NORMAL
} sys_feedback_mode_t;

types_error_code_e sys_feedback_init(void);
void sys_feedback_set_update_mode(void);
void sys_feedback_set_normal_mode(void);
void sys_feedback_whoiam(const uint8_t major, const uint8_t minor, const uint8_t patch);

#endif // SYS_FEEDBACK_H
