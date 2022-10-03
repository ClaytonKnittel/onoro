#ifndef _PRINT_CSI_H
#define _PRINT_CSI_H

#define __CSI_ESC "\033"
#define __CSI     __CSI_ESC "["

// Cursor up/down/right/left
#define CSI_CUU(n) __CSI #n "A"
#define CSI_CUD(n) __CSI #n "B"
#define CSI_CUR(n) __CSI #n "C"
#define CSI_CUL(n) __CSI #n "D"

// Cursor next line
#define CSI_CNL(n)    __CSI #n "E"
// Cursor previous line
#define CSI_CPL(n)    __CSI #n "F"
// Cursor horizontal absolute
#define CSI_CHA(n)    __CSI #n "G"
// Cursor position (row n, column m)
#define CSI_CHP(n, m) __CSI #n ";" #m "H"

// Cursor-relative actions, pass to following control sequences
#define CSI_CURSOR_BEFORE 0
#define CSI_CURSOR_AFTER  1
#define CSI_CURSOR_ALL    2

// Erase display
#define __CSI_ED(action) __CSI #action "J"
#define CSI_ED(action)   __CSI_ED(action)
// Erase line
#define __CSI_EL(action) __CSI #action "K"
#define CSI_EL(action)   __CSI_EL(action)

// Scroll up
#define CSI_SU(n) __CSI #n "S"
// Scroll down
#define CSI_SD(n) __CSI #n "T"

// Save current cursor position
#define CSI_SCP __CSI "s"
// Restore cursor position
#define CSI_RCP __CSI "u"

// Show cursor
#define CSI_SHOW __CSI "?25h"
// Hide cursor
#define CSI_HIDE __CSI "?25l"

#endif /* _PRINT_CSI_H */
