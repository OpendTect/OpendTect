#ifndef keyenum_h
#define keyenum_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          27/01/2006
 RCS:           $Id: keyenum.h,v 1.1 2006-09-12 18:39:23 cvskris Exp $
________________________________________________________________________

-*/

#include "enums.h"

class OD
{
public:
    enum		ButtonState 
    			{ //!< Qt's mouse/keyboard state values
			    NoButton        = 0x0000,
			    LeftButton      = 0x0001,
			    RightButton     = 0x0002,
			    MidButton       = 0x0004,
			    MouseButtonMask = 0x00ff,
			    ShiftButton     = 0x0100,
			    ControlButton   = 0x0200,
			    AltButton       = 0x0400,
			    MetaButton      = 0x0800,
			    KeyButtonMask   = 0x0fff,
			    Keypad          = 0x4000
			};
    			DeclareEnumUtils(ButtonState);
};

#endif
