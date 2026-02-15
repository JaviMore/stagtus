#ifndef LV_SD_FS_H
#define LV_SD_FS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LVGL filesystem driver for the SD card.
 * 
 * Registers an LVGL filesystem driver with the letter 'S'.
 * Files are accessed via paths like "S:busy.gif" or "S:/folder/file.gif".
 * 
 * Must be called AFTER SD_Init() and lv_init().
 */
void lv_sd_fs_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_SD_FS_H */
