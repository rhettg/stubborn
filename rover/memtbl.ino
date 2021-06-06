extern "C" {
    #include "tbl.h"
    #include "errors.h"
    #include "avr/eeprom.h"
}

uint16_t EEMEM memtbl[TBL_SIZE+1];

int saveTable(uint16_t *data, size_t data_len)
{
    // These are void* so the number of bytes is double because we are dealing in 2-byte words.
    eeprom_write_block(data, (void *)memtbl, 2*data_len);
    return 0;
}

int loadTable(uint16_t *data, size_t data_len)
{
    // These are void* so the number of bytes is double because we are dealing in 2-byte words.
    eeprom_read_block(data, (const void *)memtbl, 2*data_len);
    return 0;
}