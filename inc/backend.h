#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>

    typedef struct _stlink_backend {
        void (*close) (stlink_t * sl);
        int32_t (*exit_debug_mode) (stlink_t * sl);
        int32_t (*enter_swd_mode) (stlink_t * sl);
        int32_t (*enter_jtag_mode) (stlink_t * stl);
        int32_t (*exit_dfu_mode) (stlink_t * stl);
        int32_t (*core_id) (stlink_t * stl);
        int32_t (*reset) (stlink_t * stl);
        int32_t (*jtag_reset) (stlink_t * stl, int32_t value);
        int32_t (*run) (stlink_t * stl, enum run_type type);
        int32_t (*status) (stlink_t * stl);
        int32_t (*version) (stlink_t *sl);
        int32_t (*read_debug32) (stlink_t *sl, uint32_t addr, uint32_t *data);
        int32_t (*read_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int32_t (*write_debug32) (stlink_t *sl, uint32_t addr, uint32_t data);
        int32_t (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int32_t (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
        int32_t (*read_all_regs) (stlink_t *sl, struct stlink_reg * regp);
        int32_t (*read_reg) (stlink_t *sl, int32_t r_idx, struct stlink_reg * regp);
        int32_t (*read_all_unsupported_regs) (stlink_t *sl, struct stlink_reg *regp);
        int32_t (*read_unsupported_reg) (stlink_t *sl, int32_t r_idx, struct stlink_reg *regp);
        int32_t (*write_unsupported_reg) (stlink_t *sl, uint32_t value, int32_t idx, struct stlink_reg *regp);
        int32_t (*write_reg) (stlink_t *sl, uint32_t reg, int32_t idx);
        int32_t (*step) (stlink_t * stl);
        int32_t (*current_mode) (stlink_t * stl);
        int32_t (*force_debug) (stlink_t *sl);
        int32_t (*target_voltage) (stlink_t *sl);
        int32_t (*set_swdclk) (stlink_t * stl, int32_t freq_khz);
        int32_t (*trace_enable) (stlink_t * sl, uint32_t frequency);
        int32_t (*trace_disable) (stlink_t * sl);
        int32_t (*trace_read) (stlink_t * sl, uint8_t* buf, uint32_t size);
    } stlink_backend_t;

#endif // BACKEND_H
