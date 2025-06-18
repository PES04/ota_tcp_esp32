#ifndef TYPES_H
#define TYPES_H

typedef enum {
    SM_SUCCESS,
    SM_IN_PROGRESS,
    SM_ERROR
} types_ret_state_machine_e;

typedef enum {
    ERR_OK,
    ERR_FAIL,
    ERR_INVALID_PARAM,
    ERR_INVALID_OP
} types_error_code_e;

#endif
