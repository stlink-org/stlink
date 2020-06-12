#ifndef STLINK_BACKEND_H_
#define STLINK_BACKEND_H_

    typedef struct _stlink_backend {
        void (*close) (stlink_t * sl);
        int (*exit_debug_mode) (stlink_t * sl);
        int (*enter_swd_mode) (stlink_t * sl);
        int (*enter_jtag_mode) (stlink_t * stl);
        int (*exit_dfu_mode) (stlink_t * stl);
        int (*core_id) (stlink_t * stl);
        int (*reset) (stlink_t * stl);
        int (*jtag_reset) (stlink_t * stl, int value);
        int (*run) (stlink_t * stl);
        int (*status) (stlink_t * stl);
        int (*version) (stlink_t *sl);
        int (*read_debug32) (stlink_t *sl, uint32_t addr, uint32_t *data);
        int (*read_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*write_debug32) (stlink_t *sl, uint32_t addr, uint32_t data);
        int (*write_mem32) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*write_mem8) (stlink_t *sl, uint32_t addr, uint16_t len);
        int (*read_all_regs) (stlink_t *sl, struct stlink_reg * regp);
        int (*read_reg) (stlink_t *sl, int r_idx, struct stlink_reg * regp);
        int (*read_all_unsupported_regs) (stlink_t *sl, struct stlink_reg *regp);
        int (*read_unsupported_reg) (stlink_t *sl, int r_idx, struct stlink_reg *regp);
        int (*write_unsupported_reg) (stlink_t *sl, uint32_t value, int idx, struct stlink_reg *regp);
        int (*write_reg) (stlink_t *sl, uint32_t reg, int idx);
        int (*step) (stlink_t * stl);
        int (*current_mode) (stlink_t * stl);
        int (*force_debug) (stlink_t *sl);
        int32_t (*target_voltage) (stlink_t *sl);
        int (*set_swdclk) (stlink_t * stl, uint16_t divisor);
    } stlink_backend_t;

#endif // STLINK_BACKEND_H_
