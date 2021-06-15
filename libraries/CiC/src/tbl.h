#ifndef TBL_h
#define TBL_h
#include <stdint.h>
#include <stddef.h>

// TBL_SIZE defines the maximum number of table variables.
#define TBL_SIZE 8

// TBL_MAGIC is a constant stored along with the persistent data to help ensure the data 
// is not just random noise.
// TODO: Versioning
#define TBL_MAGIC 0xF0AA

#define ERR_TBL_INIT       -3
#define ERR_TBL_LOADED     -4
#define ERR_TBL_NOT_LOADED -5
#define ERR_TBL_VAR        -6

/*
 * TBL_save_func_t defines a function pointer for the user to define how
 * persistent TBL data should be stored.
 * 
 * Implementers are expected to take the data and save it somewhere persistent.
 * 
 * @param data Pointer to data to save
 * @param data_len Length of data to save
 * 
 * @returns 0 on success
 */
typedef int (*TBL_save_func_t)(const void *data, size_t data_len);

/*
 * TBL_load_func_t defines a function pointer for the user to define how to load
 * persistent TBL data.
 * 
 * Implementers are expected to write the data to the provided destination not
 * to exceed the provided buffer size.
 * 
 * @param data Destination to load the data to.
 * @param data_len Buffer size to load.
 * 
 * @returns 0 on success
 */
typedef int (*TBL_load_func_t)(void *data, size_t data_len);

/* 
 * TBL_t defines the Table Subsystem. 
 * The system is for storing a persistent table of data useful for configuration
 * variables that are expected to survive a reboot.
 * 
 * The TBL consists of a number of user defined variables represented by a
 * uint8_t. Each of these variables can store a uint16_t.
 *
 * The actual saving and loading of the data is abstracted out so as to be
 * hardware agnostic.
 * See also TBL_save_func_t and TBL_load_func_t
 */
typedef struct {
    uint16_t buf[TBL_SIZE+1];

    TBL_save_func_t save; 
    TBL_load_func_t load; 
} TBL_t;

/*
 * TBL_init initializes the table structure with the provided save and load
 * functions.
 *
 * See also TBL_save_func_t, TBL_load_func_t
 */
void TBL_init(TBL_t *tbl, TBL_save_func_t save, TBL_load_func_t load);

/*
 * TBL_set_defaults sets a default value of the specified tbl variable.
 *
 * If the variable already exists in persistent storage the value will not
 * be replaced. Calling this function will trigger no loading or saving of 
 * any data.
 *
 * @param tbl Table instance
 * @param tbl_var Specific table variable to set
 * param value Value to set
 *
 */
int TBL_set_default(TBL_t *tbl, uint8_t tbl_var, uint16_t value);

/*
 * TBL_set sets the value of the specified tbl variable.
 *
 * The new value will be immediately written to persistent storage.
 *
 * @param tbl Table instance
 * @param tbl_var Specific table variable to set
 * param value Value to set
 *
 */
int TBL_set(TBL_t *tbl, uint8_t tbl_var, uint16_t value);

/*
 * TBL_get retrieves the value of the specified tbl variable.
 *
 * @param tbl Table instance
 * @param tbl_var Specific table variable to get
 * param value Pointer to store the value to
 *
 */
int TBL_get(TBL_t *tbl, uint8_t tbl_var, uint16_t *value);

#endif
