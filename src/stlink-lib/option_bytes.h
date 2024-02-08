/*
 * File: option_bytes.h
 *
 * Read and write option bytes and option control registers
 */

#ifndef OPTION_BYTES_H
#define OPTION_BYTES_H

int32_t stlink_read_option_control_register_f0(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f0(stlink_t *sl, stm32_addr_t addr, uint8_t* base, uint32_t len);
// static int32_t stlink_write_option_control_register_f0(stlink_t *sl, uint32_t option_cr);
int32_t stlink_read_option_control_register_f2(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_read_option_bytes_f2(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_read_option_control_register_f4(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_read_option_bytes_f4(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f4(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
int32_t stlink_read_option_bytes_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_f7(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
int32_t stlink_read_option_control_register_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register_f7(stlink_t *sl, uint32_t option_cr);
int32_t stlink_read_option_control_register1_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register1_f7(stlink_t *sl, uint32_t option_cr1);
int32_t stlink_read_option_bytes_boot_add_f7(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_boot_add_f7(stlink_t *sl, uint32_t option_byte_boot_add);
int32_t stlink_read_option_control_register_gx(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_read_option_bytes_gx(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_bytes_gx(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_h7(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_l0(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_l4(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
// static int32_t stlink_write_option_bytes_wb(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
int32_t stlink_read_option_control_register_wb(stlink_t *sl, uint32_t *option_byte);
// static int32_t stlink_write_option_control_register_wb(stlink_t *sl, uint32_t option_cr);
int32_t stlink_read_option_bytes_generic(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_write_option_bytes(stlink_t *sl, stm32_addr_t addr, uint8_t *base, uint32_t len);
int32_t stlink_fwrite_option_bytes(stlink_t *sl, const char *path, stm32_addr_t addr);
int32_t stlink_read_option_control_register32(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_write_option_control_register32(stlink_t *sl, uint32_t option_cr);
int32_t stlink_read_option_control_register1_32(stlink_t *sl, uint32_t *option_byte);
int32_t stlink_write_option_control_register1_32(stlink_t *sl, uint32_t option_cr1);
int32_t stlink_read_option_bytes32(stlink_t *sl, uint32_t* option_byte);
int32_t stlink_write_option_bytes32(stlink_t *sl, uint32_t option_byte);
int32_t stlink_read_option_bytes_boot_add32(stlink_t *sl, uint32_t* option_byte);
int32_t stlink_write_option_bytes_boot_add32(stlink_t *sl, uint32_t option_bytes_boot_add);

#endif // OPTION_BYTES_H
