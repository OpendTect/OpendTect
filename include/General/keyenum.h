#ifndef keyenum_h
#define keyenum_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          27/01/2006
 RCS:           $Id: keyenum.h,v 1.4 2008-03-17 21:03:51 cvskris Exp $
________________________________________________________________________

-*/

#include "enums.h"

namespace OD
{
    enum ButtonState 
    { //!< Qt's mouse/keyboard state values
			NoButton        = 0x00000000,
			LeftButton      = 0x00000001,
			RightButton     = 0x00000002,
			MidButton       = 0x00000004,
			MouseButtonMask = 0x000000ff,
			ShiftButton     = 0x02000000,
			ControlButton   = 0x04000000,
			AltButton       = 0x08000000,
			MetaButton      = 0x10000000,
			KeyButtonMask   = 0xfe000000,
			Keypad          = 0x20000000
    };

    const char*		nameOf(ButtonState);
    ButtonState		stateOf(const char*);

    bool		leftMouseButton(ButtonState);
    bool		middleMouseButton(ButtonState);
    bool		rightMouseButton(ButtonState);
    bool		shiftKeyboardButton(ButtonState);
    bool		ctrlKeyboardButton(ButtonState);
    bool		altKeyboardButton(ButtonState);
};

#endif
