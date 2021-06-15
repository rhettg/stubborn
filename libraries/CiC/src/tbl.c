#include "tbl.h"
#include <string.h>

void TBL_init(TBL_t *tbl, TBL_save_func_t save, TBL_load_func_t load)
{
    tbl->save = save;
    tbl->load = load;
    memset(tbl->buf, 0, sizeof(tbl->buf));
}

int TBL_set_default(TBL_t *tbl, uint8_t tbl_var, uint16_t value)
{
    if (TBL_MAGIC == tbl->buf[0]) {
        return 0;
    }

    if (tbl_var == 0 || tbl_var > TBL_SIZE) {
        return ERR_TBL_VAR;
    }

    tbl->buf[tbl_var] = value;

    return 0;
}

int TBL_set(TBL_t *tbl, uint8_t tbl_var, uint16_t value)
{
    if (tbl_var == 0 || tbl_var > TBL_SIZE) {
        return ERR_TBL_VAR;
    }

    if (tbl->buf[0] != TBL_MAGIC) {
        int loaded = TBL_load(tbl);
        if (0 != loaded) {
            return loaded;
        }
    }

    tbl->buf[tbl_var] = value;

    return TBL_save(tbl);
}

int TBL_get(TBL_t *tbl, uint8_t tbl_var, uint16_t *value)
{
    int loaded = TBL_load(tbl);
    if (ERR_TBL_LOADED != loaded && 0 != loaded) {
        return loaded;
    }

    if (tbl_var == 0 || tbl_var > TBL_SIZE) {
        return ERR_TBL_VAR;
    }

    *value = tbl->buf[tbl_var];

    return 0;
}

int TBL_save(TBL_t *tbl)
{
    if (TBL_MAGIC != tbl->buf[0]) {
        return ERR_TBL_NOT_LOADED;
    }

    return tbl->save(tbl->buf, sizeof(tbl->buf));
}

int TBL_load(TBL_t *tbl)
{
    uint16_t tmp_buffer[TBL_SIZE+1];

    if (TBL_MAGIC == tbl->buf[0]) {
        // We've already been loaded
        return ERR_TBL_LOADED;
    }

    if (0 == tbl->load || 0 == tbl->save) {
        return ERR_TBL_INIT;
    }

    int loaded = tbl->load(tmp_buffer, sizeof(tmp_buffer));
    if (0 != loaded) {
        return loaded;
    }

    if (TBL_MAGIC == tmp_buffer[0]) {
        // The data we loaded is valid, put it into place.
        memcpy(tbl->buf, tmp_buffer, sizeof(tbl->buf));
    } else {
        // No saved data yet, we can just keep our defaults by marking magic.
        tbl->buf[0] = TBL_MAGIC;
    }

    return 0;
}