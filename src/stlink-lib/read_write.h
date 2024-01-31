/*
 * File: read_write.h
 *
 * Read and write operations
 */

#ifndef READ_WRITE_H
#define READ_WRITE_H

uint16_t read_uint16(const unsigned char *c, const int32_t pt);
void write_uint16(unsigned char *buf, uint16_t ui);
uint32_t read_uint32(const unsigned char *c, const int32_t pt);
void write_uint32(unsigned char *buf, uint32_t ui);

int32_t stlink_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data);
int32_t stlink_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data);
int32_t stlink_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t stlink_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t stlink_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len);
int32_t stlink_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
int32_t stlink_write_reg(stlink_t *sl, uint32_t reg, int32_t idx);
int32_t stlink_read_unsupported_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
int32_t stlink_write_unsupported_reg(stlink_t *sl, uint32_t value, int32_t r_idx, struct stlink_reg *regp);
int32_t stlink_read_all_regs(stlink_t *sl, struct stlink_reg *regp);
int32_t stlink_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp);

#endif // READ_WRITE_H
