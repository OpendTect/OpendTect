/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          30/01/2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "keyenum.h"

static const char* nms[] = 
{
	"----",
	"Left Mouse",
	"Right Mouse",
	"Mid Mouse",
	"Button Mask/Mouse",
	"Shift",
	"Control",
	"Alt",
	"Meta key",
	"Button Mask/Key",
	"Keypad",
	0
};

const int transtbl[] =
{ 0x00000000, 0x00000001, 0x00000002, 0x00000004, 0x000000ff, 0x02000000,
  0x04000000, 0x08000000, 0x10000000, 0xfe000000, 0x20000000 };


#define mImplButtonStateFunc( func, buttenum ) \
bool OD::func( OD::ButtonState st ) \
{ \
    const unsigned int state = (unsigned int) st; \
    const unsigned int mask = (unsigned int) buttenum; \
    return (state & mask); \
}

mImplButtonStateFunc( leftMouseButton, LeftButton );
mImplButtonStateFunc( middleMouseButton, MidButton );
mImplButtonStateFunc( rightMouseButton, RightButton );
mImplButtonStateFunc( shiftKeyboardButton, ShiftButton );
mImplButtonStateFunc( ctrlKeyboardButton, ControlButton );
mImplButtonStateFunc( altKeyboardButton, AltButton );


OD::ButtonState OD::stateOf( const char* nm )
{
    if ( !nm || !*nm ) return OD::NoButton;

    for ( int idx=0; nms[idx]; idx++ )
    {
	if ( !strcmp(nms[idx],nm) )
	    return (OD::ButtonState)(transtbl[idx]);
    }

    for ( int idx=0; nms[idx]; idx++ )
    {
	if ( *nms[idx] == *nm )
	    return (OD::ButtonState)(transtbl[idx]);
    }

    return OD::NoButton;
}


const char* OD::nameOf( OD::ButtonState state )
{
    for ( int idx=0; nms[idx]; idx++ )
    {
	if ( transtbl[idx] == (int)state )
	    return nms[idx];
    }
    return nms[0];
}
