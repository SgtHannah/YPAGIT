#ifndef INPUT_KEYMAP_H
#define INPUT_KEYMAP_H
/*
**  $Source: PRG:VFM/Include/input/keymap.h,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 13:00:16 $
**  $Locker:  $
**  $Author: floh $
**
**  Die system-spezifischen Keyboard-Rawcodes als
**  Macros.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef NUCLEUS_SYS_H
#include "nucleus/sys.h"
#endif

/*=================================================================**
**  Host System Amiga                                              **
**  ~~~~~~~~~~~~~~~~~                                              **
**=================================================================*/
#ifdef AMIGA

#define KEYCODE_NOP          0xff

#define KEYCODE_ESCAPE       0x45
#define KEYCODE_SPACEBAR     0x40

#define KEYCODE_CURSOR_UP    0x4c
#define KEYCODE_CURSOR_DOWN  0x4d
#define KEYCODE_CURSOR_LEFT  0x4f
#define KEYCODE_CURSOR_RIGHT 0x4e

#define KEYCODE_F1           0x50
#define KEYCODE_F2           0x51
#define KEYCODE_F3           0x52
#define KEYCODE_F4           0x53
#define KEYCODE_F5           0x54
#define KEYCODE_F6           0x55
#define KEYCODE_F7           0x56
#define KEYCODE_F8           0x57
#define KEYCODE_F9           0x58
#define KEYCODE_F10          0x59
#define KEYCODE_F11          0x0
#define KEYCODE_F12          0x0

#define KEYCODE_BS           0x41
#define KEYCODE_TILDE        0x00
#define KEYCODE_TAB          0x42
#define KEYCODE_CTRL         0x63
#define KEYCODE_LSHIFT       0x60
#define KEYCODE_RSHIFT       0x61
#define KEYCODE_LALT         0x64
#define KEYCODE_RALT         0x65
#define KEYCODE_RETURN       0x44
#define KEYCODE_INS          0x0
#define KEYCODE_DEL          0x46
#define KEYCODE_HELP         0x5f
#define KEYCODE_CLEAR        0x0
#define KEYCODE_PAUSE        0x0

#define KEYCODE_PAGEUP          0x0
#define KEYCODE_PAGEDOWN        0x0
#define KEYCODE_END             0x0
#define KEYCODE_HOME            0x0
#define KEYCODE_SELECT          0x0
#define KEYCODE_EXECUTE         0x0
#define KEYCODE_SNAPSHOT        0x0

#define KEYCODE_1            0x01
#define KEYCODE_2            0x02
#define KEYCODE_3            0x03
#define KEYCODE_4            0x04
#define KEYCODE_5            0x05
#define KEYCODE_6            0x06
#define KEYCODE_7            0x07
#define KEYCODE_8            0x08
#define KEYCODE_9            0x09
#define KEYCODE_0            0x0a

#define KEYCODE_Q            0x10
#define KEYCODE_W            0x11
#define KEYCODE_E            0x12
#define KEYCODE_R            0x13
#define KEYCODE_T            0x14
#define KEYCODE_Z            0x15   /* deutsch/american*/
#define KEYCODE_U            0x16
#define KEYCODE_I            0x17
#define KEYCODE_O            0x18
#define KEYCODE_P            0x19

#define KEYCODE_A            0x20
#define KEYCODE_S            0x21
#define KEYCODE_D            0x22
#define KEYCODE_F            0x23
#define KEYCODE_G            0x24
#define KEYCODE_H            0x25
#define KEYCODE_J            0x26
#define KEYCODE_K            0x27
#define KEYCODE_L            0x28

#define KEYCODE_Y            0x31   /* american/deutsch */
#define KEYCODE_X            0x32
#define KEYCODE_C            0x33
#define KEYCODE_V            0x34
#define KEYCODE_B            0x35
#define KEYCODE_N            0x36
#define KEYCODE_M            0x37

#define KEYCODE_NUM_LPAR     0x5a   /* [ */
#define KEYCODE_NUM_RPAR     0x5b   /* ] */
#define KEYCODE_NUM_DIV      0x5c   /* / */
#define KEYCODE_NUM_MUL      0x5d   /* * */
#define KEYCODE_NUM_MINUS    0x4a   /* - */
#define KEYCODE_NUM_PLUS     0x5e   /* + */
#define KEYCODE_ENTER        0x43
#define KEYCODE_NUM_0        0x0f
#define KEYCODE_NUM_1        0x1d
#define KEYCODE_NUM_2        0x1e
#define KEYCODE_NUM_3        0x1f
#define KEYCODE_NUM_4        0x2d
#define KEYCODE_NUM_5        0x2e
#define KEYCODE_NUM_6        0x2f
#define KEYCODE_NUM_7        0x3d
#define KEYCODE_NUM_8        0x3e
#define KEYCODE_NUM_9        0x3f
#define KEYCODE_NUM_DOT      0x3c   /* . */

#define KEYCODE_EXTRA_1         0x0    // , auf deutschen Keyboards
#define KEYCODE_EXTRA_2         0x0    // .
#define KEYCODE_EXTRA_3         0x0    // -
#define KEYCODE_EXTRA_4         0x0    // <
#define KEYCODE_EXTRA_5         0x0    // ü
#define KEYCODE_EXTRA_6         0x0    // +
#define KEYCODE_EXTRA_7         0x0    // ö
#define KEYCODE_EXTRA_8         0x0    // ä
#define KEYCODE_EXTRA_9         0x0    // #
#define KEYCODE_EXTRA_10        0x0    // ´
#define KEYCODE_EXTRA_11        0x0    // ^

#endif

/*=================================================================**
**  Host System MS-DOS                                             **
**  ~~~~~~~~~~~~~~~~~~                                             **
**=================================================================*/

#ifdef __DOS__

#define KEYCODE_NOP             0xff

#define KEYCODE_ESCAPE          0x01
#define KEYCODE_SPACEBAR        0x39

#define KEYCODE_CURSOR_UP       0x48
#define KEYCODE_CURSOR_DOWN     0x50
#define KEYCODE_CURSOR_LEFT     0x4b
#define KEYCODE_CURSOR_RIGHT    0x4d

#define KEYCODE_F1              0x3b
#define KEYCODE_F2              0x3c
#define KEYCODE_F3              0x3d
#define KEYCODE_F4              0x3e
#define KEYCODE_F5              0x3f
#define KEYCODE_F6              0x40
#define KEYCODE_F7              0x41
#define KEYCODE_F8              0x42
#define KEYCODE_F9              0x43
#define KEYCODE_F10             0x44

#define KEYCODE_BS              0x0e
#define KEYCODE_TAB             0x0f
#define KEYCODE_CLEAR           0x0
#define KEYCODE_RETURN          0x1c
#define KEYCODE_CTRL            0x1d
#define KEYCODE_LSHIFT          0x2a
#define KEYCODE_RSHIFT          0x36
#define KEYCODE_LALT            0x38
#define KEYCODE_RALT            0x38
#define KEYCODE_PAUSE           0x0

#define KEYCODE_INS             0x0
#define KEYCODE_DEL             0x53
#define KEYCODE_HELP            0x52

#define KEYCODE_1               0x02
#define KEYCODE_2               0x03
#define KEYCODE_3               0x04
#define KEYCODE_4               0x05
#define KEYCODE_5               0x06
#define KEYCODE_6               0x07
#define KEYCODE_7               0x08
#define KEYCODE_8               0x09
#define KEYCODE_9               0x0a
#define KEYCODE_0               0x0b

#define KEYCODE_Q               0x10
#define KEYCODE_W               0x11
#define KEYCODE_E               0x12
#define KEYCODE_R               0x13
#define KEYCODE_T               0x14
#define KEYCODE_ZY              0x15    /* deutsch/american*/
#define KEYCODE_U               0x16
#define KEYCODE_I               0x17
#define KEYCODE_O               0x18
#define KEYCODE_P               0x19

#define KEYCODE_A               0x1e
#define KEYCODE_S               0x1f
#define KEYCODE_D               0x20
#define KEYCODE_F               0x21
#define KEYCODE_G               0x22
#define KEYCODE_H               0x23
#define KEYCODE_J               0x24
#define KEYCODE_K               0x25
#define KEYCODE_L               0x26

#define KEYCODE_YZ              0x2c    /* american/deutsch */
#define KEYCODE_X               0x2d
#define KEYCODE_C               0x2e
#define KEYCODE_V               0x2f
#define KEYCODE_B               0x30
#define KEYCODE_N               0x31
#define KEYCODE_M               0x32

#define KEYCODE_NUM_LPAR        0x45    /* [ */
#define KEYCODE_NUM_RPAR        0x45    /* ] */
#define KEYCODE_NUM_DIV         0x35    /* / */
#define KEYCODE_NUM_MUL         0x37    /* * */
#define KEYCODE_NUM_MINUS       0x4a    /* - */
#define KEYCODE_NUM_PLUS        0x4e    /* + */
#define KEYCODE_ENTER           0x1c
#define KEYCODE_NUM_0           0x52
#define KEYCODE_NUM_1           0x4f
#define KEYCODE_NUM_2           0x50
#define KEYCODE_NUM_3           0x51
#define KEYCODE_NUM_4           0x4b
#define KEYCODE_NUM_5           0x4c
#define KEYCODE_NUM_6           0x4d
#define KEYCODE_NUM_7           0x47
#define KEYCODE_NUM_8           0x48
#define KEYCODE_NUM_9           0x49
#define KEYCODE_NUM_DOT         0x53    /* . */

#endif

/*=================================================================**
**  Host System Windows (virtual keycodes)                         **
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                         **
**=================================================================*/

#ifdef __WINDOWS__

#define KEYCODE_NOP             0xff

#define KEYCODE_ESCAPE          0x1b
#define KEYCODE_SPACEBAR        0x20

#define KEYCODE_CURSOR_UP       0x26
#define KEYCODE_CURSOR_DOWN     0x28
#define KEYCODE_CURSOR_LEFT     0x25
#define KEYCODE_CURSOR_RIGHT    0x27

#define KEYCODE_F1              0x70
#define KEYCODE_F2              0x71
#define KEYCODE_F3              0x72
#define KEYCODE_F4              0x73
#define KEYCODE_F5              0x74
#define KEYCODE_F6              0x75
#define KEYCODE_F7              0x76
#define KEYCODE_F8              0x77
#define KEYCODE_F9              0x78
#define KEYCODE_F10             0x79
#define KEYCODE_F11             0x7a
#define KEYCODE_F12             0x7b

#define KEYCODE_BS              0x08
#define KEYCODE_TAB             0x09
#define KEYCODE_CLEAR           0x0c
#define KEYCODE_RETURN          0x0d
#define KEYCODE_CTRL            0x11
#define KEYCODE_LSHIFT          0x10        // nur 1x Shift
#define KEYCODE_RSHIFT          0x10
#define KEYCODE_LALT            0x12        // nur 1x Alt
#define KEYCODE_RALT            0x12
#define KEYCODE_PAUSE           0x13

#define KEYCODE_PAGEUP          0x21
#define KEYCODE_PAGEDOWN        0x22
#define KEYCODE_END             0x23
#define KEYCODE_HOME            0x24
#define KEYCODE_SELECT          0x29
#define KEYCODE_EXECUTE         0x2b
#define KEYCODE_SNAPSHOT        0x2c

#define KEYCODE_INS             0x2d
#define KEYCODE_DEL             0x2e
#define KEYCODE_HELP            0x2f

#define KEYCODE_1               0x31
#define KEYCODE_2               0x32
#define KEYCODE_3               0x33
#define KEYCODE_4               0x34
#define KEYCODE_5               0x35
#define KEYCODE_6               0x36
#define KEYCODE_7               0x37
#define KEYCODE_8               0x38
#define KEYCODE_9               0x39
#define KEYCODE_0               0x30

#define KEYCODE_A               0x41
#define KEYCODE_B               0x42
#define KEYCODE_C               0x43
#define KEYCODE_D               0x44
#define KEYCODE_E               0x45
#define KEYCODE_F               0x46
#define KEYCODE_G               0x47
#define KEYCODE_H               0x48
#define KEYCODE_I               0x49
#define KEYCODE_J               0x4a
#define KEYCODE_K               0x4b
#define KEYCODE_L               0x4c
#define KEYCODE_M               0x4d
#define KEYCODE_N               0x4e
#define KEYCODE_O               0x4f
#define KEYCODE_P               0x50
#define KEYCODE_Q               0x51
#define KEYCODE_R               0x52
#define KEYCODE_S               0x53
#define KEYCODE_T               0x54
#define KEYCODE_U               0x55
#define KEYCODE_V               0x56
#define KEYCODE_W               0x57
#define KEYCODE_X               0x58
#define KEYCODE_Y               0x59
#define KEYCODE_Z               0x5a

#define KEYCODE_NUM_0           0x60
#define KEYCODE_NUM_1           0x61
#define KEYCODE_NUM_2           0x62
#define KEYCODE_NUM_3           0x63
#define KEYCODE_NUM_4           0x64
#define KEYCODE_NUM_5           0x65
#define KEYCODE_NUM_6           0x66
#define KEYCODE_NUM_7           0x67
#define KEYCODE_NUM_8           0x68
#define KEYCODE_NUM_9           0x69

#define KEYCODE_NUM_MUL         0x6a
#define KEYCODE_NUM_PLUS        0x6b
#define KEYCODE_NUM_DOT         0x6c
#define KEYCODE_NUM_MINUS       0x6d
#define KEYCODE_ENTER           0x6e
#define KEYCODE_NUM_DIV         0x6f

#define KEYCODE_EXTRA_1         0xbc    // , auf deutschen Keyboards
#define KEYCODE_EXTRA_2         0xbe    // .
#define KEYCODE_EXTRA_3         0xbd    // -
#define KEYCODE_EXTRA_4         0xe2    // <
#define KEYCODE_EXTRA_5         0xba    // ü
#define KEYCODE_EXTRA_6         0xbb    // +
#define KEYCODE_EXTRA_7         0xc0    // ö
#define KEYCODE_EXTRA_8         0xde    // ä
#define KEYCODE_EXTRA_9         0xbf    // #
#define KEYCODE_EXTRA_10        0xdd    // ´
#define KEYCODE_EXTRA_11        0xdc    // ^

#define KEYCODE_NUM_LPAR        0x21    // eigentlich Page Up
#define KEYCODE_NUM_RPAR        0x22    // eigentlich Page Down

#endif

/*-----------------------------------------------------------------*/
#endif

