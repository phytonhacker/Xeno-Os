#ifndef SHELL_H
#define SHELL_H

/* Egy beérkezett karakter feldolgozása a shell input bufferében */
void shell_putchar(char c);

/* Promptot ír ki, ha a shell aktív */
void shell_prompt(void);

/* Teljes újrarajzolás - akkor hívjuk, amikor visszatérünk pl. az editorból */
void shell_redraw(void);

#endif
