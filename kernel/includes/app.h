#ifndef APP_H
#define APP_H

/* Az aktív "alkalmazás" – ide bővíthető később több is (pl. paint, calc) */
typedef enum { MODE_SHELL, MODE_EDITOR } app_mode_t;

extern app_mode_t mode;

#endif