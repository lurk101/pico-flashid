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

#include "stdinit.h"

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

static void flash_get_unique_id(uint8_t* id) {
    ((rom_flash_exit_xip_fn)rom_func_lookup(ROM_FUNC_FLASH_EXIT_XIP))();
    flash_cs_force(0);
    uint8_t buf[FLASH_RUID_TOTAL_BYTES] = {FLASH_RUID_CMD};
    uint8_t* txbuf = buf;
    uint32_t tx_remaining = FLASH_RUID_TOTAL_BYTES, rx_remaining = FLASH_RUID_TOTAL_BYTES;
    while (tx_remaining || rx_remaining) {
        if ((ssi_hw->sr & SSI_SR_TFNF_BITS) && tx_remaining) {
            ssi_hw->dr0 = *txbuf++;
            --tx_remaining;
        }
        if ((ssi_hw->sr & SSI_SR_RFNE_BITS) && rx_remaining) {
            uint8_t c = ssi_hw->dr0;
            if (rx_remaining-- <= FLASH_RUID_DATA_BYTES)
                *id++ = c;
        }
    }
    flash_cs_force(1);
    ((rom_flash_flush_cache_fn)rom_func_lookup(ROM_FUNC_FLASH_FLUSH_CACHE))();
    ((rom_flash_enter_cmd_xip_fn)rom_func_lookup(ROM_FUNC_FLASH_ENTER_CMD_XIP))();
}
#endif

int main(void) {
    stdio_init();
    uint64_t id;
    flash_get_unique_id((uint8_t*)&id);
    printf("ID %llx\n", __builtin_bswap64(id));
}
