extern "C" {
    #include "tbl.h"
    #include "errors.h"
    #include "avr/eeprom.h"
}

uint16_t EEMEM memtbl[TBL_SIZE+1];

int saveTable(const void *data, size_t data_len)
{
    eeprom_write_block(data, (void *)memtbl, data_len);
    return 0;
}

int loadTable(void *data, size_t data_len)
{
    eeprom_read_block(data, (const void *)memtbl, data_len);
    return 0;
}