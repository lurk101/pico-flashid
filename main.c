#if PICO_NO_FLASH
#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include "hardware/structs/ioqspi.h"
#include "hardware/structs/ssi.h"
#else
#include "hardware/flash.h"
#endif

#include "pico/stdio.h"

#include "stdio.h"
#include "stdlib.h"

#if PICO_NO_FLASH
#define FLASH_RUID_CMD 0x4b
#define FLASH_RUID_DUMMY_BYTES 4
#define FLASH_RUID_DATA_BYTES 8
#define FLASH_RUID_TOTAL_BYTES (1 + FLASH_RUID_DUMMY_BYTES + FLASH_RUID_DATA_BYTES)

static void flash_cs_force(bool high) {
    uint32_t field_val = high ? IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_VALUE_HIGH
                              : IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_VALUE_LOW;
    hw_write_masked(&ioqspi_hw->io[1].ctrl, field_val << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_BITS);
}

static void flash_do_cmd(const uint8_t* txbuf, uint8_t* rxbuf, size_t count) {
    ((rom_flash_exit_xip_fn)rom_func_lookup(ROM_FUNC_FLASH_EXIT_XIP))();
    flash_cs_force(0);
    uint32_t tx_remaining = count, rx_remaining = count;
    while (tx_remaining || rx_remaining) {
        if ((ssi_hw->sr & SSI_SR_TFNF_BITS) && tx_remaining) {
            ssi_hw->dr0 = *txbuf++;
            --tx_remaining;
        }
        if ((ssi_hw->sr & SSI_SR_RFNE_BITS) && rx_remaining) {
            *rxbuf++ = ssi_hw->dr0;
            --rx_remaining;
        }
    }
    flash_cs_force(1);
    ((rom_flash_flush_cache_fn)rom_func_lookup(ROM_FUNC_FLASH_FLUSH_CACHE))();
    ((rom_flash_enter_cmd_xip_fn)rom_func_lookup(ROM_FUNC_FLASH_ENTER_CMD_XIP))();
}

static void flash_get_unique_id(uint8_t* id_out) {
    uint8_t buf[FLASH_RUID_TOTAL_BYTES * 2] = {0};
    buf[0] = FLASH_RUID_CMD;
    flash_do_cmd(buf, buf + FLASH_RUID_TOTAL_BYTES, FLASH_RUID_TOTAL_BYTES);
    __builtin_memcpy(id_out, buf + 1 + FLASH_RUID_DUMMY_BYTES + FLASH_RUID_TOTAL_BYTES,
                     FLASH_RUID_DATA_BYTES);
}
#endif

int main(void) {
    stdio_init_all();
    uint64_t id;
    flash_get_unique_id((uint8_t*)&id);
    printf("ID %llx\n", __builtin_bswap64(id));
}
