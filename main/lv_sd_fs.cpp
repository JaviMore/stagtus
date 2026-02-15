/**
 * @file lv_sd_fs.cpp
 * @brief LVGL filesystem driver for Arduino SD library.
 *
 * Bridges the LVGL filesystem API (lv_fs) to the Arduino SD library,
 * allowing LVGL to load images/GIFs directly from the SD card.
 * 
 * Drive letter: 'S'
 * Usage in EEZ Studio: reference images as "S:\busy.gif" 
 * which LVGL interprets as "S:busy.gif" -> opens "/busy.gif" on SD.
 */

#include "lv_sd_fs.h"
#include <lvgl.h>
#include <SD.h>
#include <FS.h>

/* ──────────────────────── helpers ──────────────────────── */

/**
 * Build the full SD path from the LVGL path.
 * LVGL strips the drive letter, so we receive paths like:
 *   "busy.gif"  or  "/busy.gif"  or  "folder/file.gif"
 * We always prepend '/' if not present.
 */
static void build_path(const char *lvgl_path, char *out, size_t out_len)
{
    if (lvgl_path[0] == '/' || lvgl_path[0] == '\\') {
        snprintf(out, out_len, "%s", lvgl_path);
    } else {
        snprintf(out, out_len, "/%s", lvgl_path);
    }
    /* Normalize backslashes coming from EEZ Studio paths */
    for (char *p = out; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

/* ──────────────── filesystem driver callbacks ──────────── */

static bool sd_fs_ready(lv_fs_drv_t *drv)
{
    (void)drv;
    return SD.cardType() != CARD_NONE;
}

static void *sd_fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    (void)drv;

    char full_path[LV_FS_MAX_PATH_LENGTH];
    build_path(path, full_path, sizeof(full_path));

    const char *sd_mode = FILE_READ;
    if (mode == LV_FS_MODE_WR) {
        sd_mode = FILE_WRITE;
    }

    File *fp = new File();
    *fp = SD.open(full_path, sd_mode);

    if (!(*fp)) {
        delete fp;
        return NULL;
    }

    return (void *)fp;
}

static lv_fs_res_t sd_fs_close(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    File *fp = (File *)file_p;
    fp->close();
    delete fp;
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_read(lv_fs_drv_t *drv, void *file_p,
                               void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    File *fp = (File *)file_p;
    *br = fp->read((uint8_t *)buf, btr);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_write(lv_fs_drv_t *drv, void *file_p,
                                const void *buf, uint32_t btw, uint32_t *bw)
{
    (void)drv;
    File *fp = (File *)file_p;
    *bw = fp->write((const uint8_t *)buf, btw);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_seek(lv_fs_drv_t *drv, void *file_p,
                               uint32_t pos, lv_fs_whence_t whence)
{
    (void)drv;
    File *fp = (File *)file_p;

    switch (whence) {
        case LV_FS_SEEK_SET:
            fp->seek(pos, SeekSet);
            break;
        case LV_FS_SEEK_CUR:
            fp->seek(pos, SeekCur);
            break;
        case LV_FS_SEEK_END:
            fp->seek(pos, SeekEnd);
            break;
        default:
            return LV_FS_RES_INV_PARAM;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    File *fp = (File *)file_p;
    *pos_p = fp->position();
    return LV_FS_RES_OK;
}

/* ──────────────────── public API ──────────────────────── */

static lv_fs_drv_t sd_drv;

void lv_sd_fs_init(void)
{
    lv_fs_drv_init(&sd_drv);

    sd_drv.letter     = 'S';
    sd_drv.cache_size = 512;   /* Read-ahead cache for faster sequential reads */
    sd_drv.ready_cb   = sd_fs_ready;
    sd_drv.open_cb    = sd_fs_open;
    sd_drv.close_cb   = sd_fs_close;
    sd_drv.read_cb    = sd_fs_read;
    sd_drv.write_cb   = sd_fs_write;
    sd_drv.seek_cb    = sd_fs_seek;
    sd_drv.tell_cb    = sd_fs_tell;

    lv_fs_drv_register(&sd_drv);
}
