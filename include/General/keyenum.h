#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"

namespace OD
{
    enum ButtonState
    { //!< Qt's mouse/keyboard state values
	NoButton	= 0x00000000,
	LeftButton	= 0x00000001,
	RightButton	= 0x00000002,
	MidButton	= 0x00000004,
	MouseButtonMask	= 0x000000ff,
	ShiftButton	= 0x02000000,
	ControlButton	= 0x04000000,
	AltButton	= 0x08000000,
	MetaButton	= 0x10000000,
	KeyButtonMask	= 0xfe000000,
	Keypad		= 0x20000000
    };

    enum KeyboardKey
    {
	KB_NoKey		= 0x00000000,
	KB_Escape		= 0x01000000,
	KB_Tab			= 0x01000001,
	KB_Backtab		= 0x01000002,
	KB_Backspace		= 0x01000003,
	KB_Return		= 0x01000004,
	KB_Enter		= 0x01000005,
	KB_Insert		= 0x01000006,
	KB_Delete		= 0x01000007,
	KB_Pause		= 0x01000008,
	KB_Print		= 0x01000009,
	KB_SysReq		= 0x0100000a,
	KB_Clear		= 0x0100000b,
	KB_Home			= 0x01000010,
	KB_End			= 0x01000011,
	KB_Left			= 0x01000012,
	KB_Up			= 0x01000013,
	KB_Right		= 0x01000014,
	KB_Down			= 0x01000015,
	KB_PageUp		= 0x01000016,
	KB_PageDown		= 0x01000017,
	KB_Shift		= 0x01000020,
	KB_Control		= 0x01000021, // Mac OS X: Command keys.
	KB_Meta			= 0x01000022, // Mac OS X: Control keys.
	KB_Alt			= 0x01000023,
	KB_AltGr		= 0x01001103,
	KB_CapsLock		= 0x01000024,
	KB_NumLock		= 0x01000025,
	KB_ScrollLock		= 0x01000026,
	KB_F1			= 0x01000030,
	KB_F2			= 0x01000031,
	KB_F3			= 0x01000032,
	KB_F4			= 0x01000033,
	KB_F5			= 0x01000034,
	KB_F6			= 0x01000035,
	KB_F7			= 0x01000036,
	KB_F8			= 0x01000037,
	KB_F9			= 0x01000038,
	KB_F10			= 0x01000039,
	KB_F11			= 0x0100003a,
	KB_F12			= 0x0100003b,
	KB_F13			= 0x0100003c,
	KB_F14			= 0x0100003d,
	KB_F15			= 0x0100003e,
	KB_F16			= 0x0100003f,
	KB_F17			= 0x01000040,
	KB_F18			= 0x01000041,
	KB_F19			= 0x01000042,
	KB_F20			= 0x01000043,
	KB_F21			= 0x01000044,
	KB_F22			= 0x01000045,
	KB_F23			= 0x01000046,
	KB_F24			= 0x01000047,
	KB_F25			= 0x01000048,
	KB_F26			= 0x01000049,
	KB_F27			= 0x0100004a,
	KB_F28			= 0x0100004b,
	KB_F29			= 0x0100004c,
	KB_F30			= 0x0100004d,
	KB_F31			= 0x0100004e,
	KB_F32			= 0x0100004f,
	KB_F33			= 0x01000050,
	KB_F34			= 0x01000051,
	KB_F35			= 0x01000052,
	KB_Super_L		= 0x01000053,
	KB_Super_R		= 0x01000054,
	KB_Menu			= 0x01000055,
	KB_Hyper_L		= 0x01000056,
	KB_Hyper_R		= 0x01000057,
	KB_Help			= 0x01000058,
	KB_Direction_L		= 0x01000059,
	KB_Direction_R		= 0x01000060,
	KB_Space		= 0x20,
	KB_Any			= KB_Space,
	KB_Exclam		= 0x21,
	KB_QuoteDbl		= 0x22,
	KB_NumberSign		= 0x23,
	KB_Dollar		= 0x24,
	KB_Percent		= 0x25,
	KB_Ampersand		= 0x26,
	KB_Apostrophe		= 0x27,
	KB_ParenLeft		= 0x28,
	KB_ParenRight		= 0x29,
	KB_Asterisk		= 0x2a,
	KB_Plus			= 0x2b,
	KB_Comma		= 0x2c,
	KB_Minus		= 0x2d,
	KB_Period		= 0x2e,
	KB_Slash		= 0x2f,
	KB_Zero			= 0x30,
	KB_One			= 0x31,
	KB_Two			= 0x32,
	KB_Three		= 0x33,
	KB_Four			= 0x34,
	KB_Five			= 0x35,
	KB_Six			= 0x36,
	KB_Seven		= 0x37,
	KB_Eight		= 0x38,
	KB_Nine			= 0x39,
	KB_Colon		= 0x3a,
	KB_Semicolon		= 0x3b,
	KB_Less			= 0x3c,
	KB_Equal		= 0x3d,
	KB_Greater		= 0x3e,
	KB_Question		= 0x3f,
	KB_At			= 0x40,
	KB_A			= 0x41,
	KB_B			= 0x42,
	KB_C			= 0x43,
	KB_D			= 0x44,
	KB_E			= 0x45,
	KB_F			= 0x46,
	KB_G			= 0x47,
	KB_H			= 0x48,
	KB_I			= 0x49,
	KB_J			= 0x4a,
	KB_K			= 0x4b,
	KB_L			= 0x4c,
	KB_M			= 0x4d,
	KB_N			= 0x4e,
	KB_O			= 0x4f,
	KB_P			= 0x50,
	KB_Q			= 0x51,
	KB_R			= 0x52,
	KB_S			= 0x53,
	KB_T			= 0x54,
	KB_U			= 0x55,
	KB_V			= 0x56,
	KB_W			= 0x57,
	KB_X			= 0x58,
	KB_Y			= 0x59,
	KB_Z			= 0x5a,
	KB_BracketLeft		= 0x5b,
	KB_Backslash		= 0x5c,
	KB_BracketRight		= 0x5d,
	KB_AsciiCircum		= 0x5e,
	KB_Underscore		= 0x5f,
	KB_QuoteLeft		= 0x60,
	KB_BraceLeft		= 0x7b,
	KB_Bar			= 0x7c,
	KB_BraceRight		= 0x7d,
	KB_AsciiTilde		= 0x7e,
	KB_Nobreakspace		= 0x0a0,
	KB_Exclamdown		= 0x0a1,
	KB_Cent			= 0x0a2,
	KB_Sterling		= 0x0a3,
	KB_Currency		= 0x0a4,
	KB_Yen			= 0x0a5,
	KB_Brokenbar		= 0x0a6,
	KB_Section		= 0x0a7,
	KB_Diaeresis		= 0x0a8,
	KB_Copyright		= 0x0a9,
	KB_Ordfeminine		= 0x0aa,
	KB_Guillemotleft	= 0x0ab,
	KB_Notsign		= 0x0ac,
	KB_Hyphen		= 0x0ad,
	KB_Registered		= 0x0ae,
	KB_Macron		= 0x0af,
	KB_Degree		= 0x0b0,
	KB_Plusminus		= 0x0b1,
	KB_Twosuperior		= 0x0b2,
	KB_Threesuperior	= 0x0b3,
	KB_Acute		= 0x0b4,
	KB_Mu			= 0x0b5,
	KB_Paragraph		= 0x0b6,
	KB_Periodcentered	= 0x0b7,
	KB_Cedilla		= 0x0b8,
	KB_Onesuperior		= 0x0b9,
	KB_Masculine		= 0x0ba,
	KB_Guillemotright	= 0x0bb,
	KB_Onequarter		= 0x0bc,
	KB_Onehalf		= 0x0bd,
	KB_Threequarters	= 0x0be,
	KB_Questiondown		= 0x0bf,
	KB_agrave		= 0x0c0,
	KB_Aacute		= 0x0c1,
	KB_Acircumflex		= 0x0c2,
	KB_Atilde		= 0x0c3,
	KB_Adiaeresis		= 0x0c4,
	KB_Aring		= 0x0c5,
	KB_AE			= 0x0c6,
	KB_Ccedilla		= 0x0c7,
	KB_Egrave		= 0x0c8,
	KB_Eacute		= 0x0c9,
	KB_Ecircumflex		= 0x0ca,
	KB_Ediaeresis		= 0x0cb,
	KB_Igrave		= 0x0cc,
	KB_Iacute		= 0x0cd,
	KB_Icircumflex		= 0x0ce,
	KB_Idiaeresis		= 0x0cf,
	KB_ETH			= 0x0d0,
	KB_NTilde		= 0x0d1,
	KB_Ograve		= 0x0d2,
	KB_Oacute		= 0x0d3,
	KB_Ocircumflex		= 0x0d4,
	KB_Otilde		= 0x0d5,
	KB_Odiaeresis		= 0x0d6,
	KB_Multiply		= 0x0d7,
	KB_Ooblique		= 0x0d8,
	KB_Ugrave		= 0x0d9,
	KB_Uacute		= 0x0da,
	KB_Ucircumflex		= 0x0db,
	KB_Udiaeresis		= 0x0dc,
	KB_Yacute		= 0x0dd,
	KB_THORN		= 0x0de,
	KB_Ssharp		= 0x0df,
	KB_Division		= 0x0f7,
	KB_Ydiaeresis		= 0x0ff,
	KB_Multi_key		= 0x01001120,
	KB_Codeinput		= 0x01001137,
	KB_SingleCandidate	= 0x0100113c,
	KB_MultipleCandidate	= 0x0100113d,
	KB_PreviousCandidate	= 0x0100113e,
	KB_Mode_switch		= 0x0100117e,
	KB_Kanji		= 0x01001121,
	KB_Muhenkan		= 0x01001122,
	KB_Henkan		= 0x01001123,
	KB_Romaji		= 0x01001124,
	KB_Hiragana		= 0x01001125,
	KB_Katakana		= 0x01001126,
	KB_Hiragana_Katakana	= 0x01001127,
	KB_Zenkaku		= 0x01001128,
	KB_Hankaku		= 0x01001129,
	KB_Zenkaku_Hankaku	= 0x0100112a,
	KB_Touroku		= 0x0100112b,
	KB_Massyo		= 0x0100112c,
	KB_Kana_Lock		= 0x0100112d,
	KB_Kana_Shift		= 0x0100112e,
	KB_Eisu_Shift		= 0x0100112f,
	KB_Eisu_toggle		= 0x01001130,
	KB_Hangul		= 0x01001131,
	KB_Hangul_Start		= 0x01001132,
	KB_Hangul_End		= 0x01001133,
	KB_Hangul_Hanja		= 0x01001134,
	KB_Hangul_Jamo		= 0x01001135,
	KB_Hangul_Romaja	= 0x01001136,
	KB_Hangul_Jeonja	= 0x01001138,
	KB_Hangul_Banja		= 0x01001139,
	KB_Hangul_PreHanja	= 0x0100113a,
	KB_Hangul_PostHanja	= 0x0100113b,
	KB_Hangul_Special	= 0x0100113f,
	KB_Dead_Grave		= 0x01001250,
	KB_Dead_Acute		= 0x01001251,
	KB_Dead_Circumflex	= 0x01001252,
	KB_Dead_Tilde		= 0x01001253,
	KB_Dead_Macron		= 0x01001254,
	KB_Dead_Breve		= 0x01001255,
	KB_Dead_Abovedot	= 0x01001256,
	KB_Dead_Diaeresis	= 0x01001257,
	KB_Dead_Abovering	= 0x01001258,
	KB_Dead_Doubleacute	= 0x01001259,
	KB_Dead_Caron		= 0x0100125a,
	KB_Dead_Cedilla		= 0x0100125b,
	KB_Dead_Ogonek		= 0x0100125c,
	KB_Dead_Iota		= 0x0100125d,
	KB_Dead_Voiced_Sound	= 0x0100125e,
	KB_Dead_Semivoiced_Sound= 0x0100125f,
	KB_Dead_Belowdot	= 0x01001260,
	KB_Dead_Hook		= 0x01001261,
	KB_Dead_Horn		= 0x01001262,
	KB_Back			= 0x01000061,
	KB_Forward		= 0x01000062,
	KB_Stop			= 0x01000063,
	KB_Refresh		= 0x01000064,
	KB_VolumeDown		= 0x01000070,
	KB_VolumeMute		= 0x01000071,
	KB_VolumeUp		= 0x01000072,
	KB_BassBoost		= 0x01000073,
	KB_BassUp		= 0x01000074,
	KB_BassDown		= 0x01000075,
	KB_TrebleUp		= 0x01000076,
	KB_TrebleDown		= 0x01000077,
	KB_MediaPlay		= 0x01000080,
	KB_MediaStop		= 0x01000081,
	KB_MediaPrevious	= 0x01000082,
	KB_MediaNext		= 0x01000083,
	KB_MediaRecord		= 0x01000084,
	KB_HomePage		= 0x01000090,
	KB_Favorites		= 0x01000091,
	KB_Search		= 0x01000092,
	KB_Standby		= 0x01000093,
	KB_OpenUrl		= 0x01000094,
	KB_LaunchMail		= 0x010000a0,
	KB_LaunchMedia		= 0x010000a1,
	KB_Launch0		= 0x010000a2,
	KB_Launch1		= 0x010000a3,
	KB_Launch2		= 0x010000a4,
	KB_Launch3		= 0x010000a5,
	KB_Launch4		= 0x010000a6,
	KB_Launch5		= 0x010000a7,
	KB_Launch6		= 0x010000a8,
	KB_Launch7		= 0x010000a9,
	KB_Launch8		= 0x010000aa,
	KB_Launch9		= 0x010000ab,
	KB_LaunchA		= 0x010000ac,
	KB_LaunchB		= 0x010000ad,
	KB_LaunchC		= 0x010000ae,
	KB_LaunchD		= 0x010000af,
	KB_LaunchE		= 0x010000b0,
	KB_LaunchF		= 0x010000b1,
	KB_MediaLast		= 0x0100ffff,
	KB_unknown		= 0x01ffffff,
	KB_Call			= 0x01100004,
	KB_Context1		= 0x01100000,
	KB_Context2		= 0x01100001,
	KB_Context3		= 0x01100002,
	KB_Context4		= 0x01100003,
	KB_Flip			= 0x01100006,
	KB_Hangup		= 0x01100005,
	KB_No			= 0x01010002,
	KB_Select		= 0x01010000,
	KB_Yes			= 0x01010001,
	KB_Execute		= 0x01020003,
	KB_Printer		= 0x01020002,
	KB_Play			= 0x01020005,
	KB_Sleep		= 0x01020004,
	KB_Zoom			= 0x01020006,
	KB_Cancel		= 0x01020001
    };

    mGlobal(General) const char*	nameOf(ButtonState);
    mGlobal(General) ButtonState	stateOf(const char*);

    mGlobal(General) bool		leftMouseButton(ButtonState);
    mGlobal(General) bool		middleMouseButton(ButtonState);
    mGlobal(General) bool		rightMouseButton(ButtonState);
    mGlobal(General) bool		shiftKeyboardButton(ButtonState);
    mGlobal(General) bool		ctrlKeyboardButton(ButtonState);
    mGlobal(General) bool		altKeyboardButton(ButtonState);

} // namespace OD
