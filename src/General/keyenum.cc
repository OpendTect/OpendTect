/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          30/01/2006
 RCS:           $Id: keyenum.cc,v 1.2 2007-03-15 16:15:06 cvsbert Exp $
________________________________________________________________________

-*/


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
