#ifndef TBL_h
#define TBL_h
#include <stdint.h>
#include <stddef.h>

#define TBL_SIZE 8
#define TBL_MAGIC 0xF0AA

#define ERR_TBL_INIT -3
#define ERR_TBL_LOADED -4
#define ERR_TBL_NOT_LOADED -5
#define ERR_TBL_VAR -6

typedef int (*TBL_save_func_t)(const void *data, size_t data_len);
typedef int (*TBL_load_func_t)(void *data, size_t data_len);

// TBL_t defines the Table Subsystem. 
// The system is for storing a persistent table of data useful for configuration
// variables that are expected to survive a reboot.
typedef struct {
    uint16_t buf[TBL_SIZE+1];

    TBL_save_func_t save; 
    TBL_load_func_t load; 
} TBL_t;

void TBL_init(TBL_t *tbl, TBL_save_func_t save, TBL_load_func_t load);

int TBL_set(TBL_t *tbl, uint8_t tbl_var, uint16_t value);
int TBL_set_default(TBL_t *tbl, uint8_t tbl_var, uint16_t value);
int TBL_get(TBL_t *tbl, uint8_t tbl_var, uint16_t *value);
#endif