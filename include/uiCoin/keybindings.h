/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Dec 2002
 RCS:           $Id: keybindings.h,v 1.1 2003-01-13 12:32:37 nanne Exp $
________________________________________________________________________

*/

#include "bufstring.h"


/*! \brief Class for setting keybindings.

Each binding is a BufferString. This string contains the several keys 
separated by a `.  e.g. zoom = "Left`Control"

*/


class KeyBindings
{
public:
    				KeyBindings(const char* nm=0)
				    : name(nm) {};

    BufferString		name;

    BufferString		zoom;
    BufferString		rotate;
    BufferString		pan;


    static const char*		sName;
    static const char*		sRotate;
    static const char*		sPan;
    static const char*		sZoom;

    static const char*		sControl;
    static const char*		sShift;
    static const char*		sRight;
    static const char*		sLeft;
    static const char*		sMiddle;
    static const char*		sNone;
};


const char* KeyBindings::sName = 	"Name";
const char* KeyBindings::sRotate = 	"Rotate";
const char* KeyBindings::sPan = 	"Pan";
const char* KeyBindings::sZoom = 	"Zoom";
const char* KeyBindings::sControl = 	"Control";
const char* KeyBindings::sShift = 	"Shift";
const char* KeyBindings::sLeft = 	"Left";
const char* KeyBindings::sRight = 	"Right";
const char* KeyBindings::sMiddle = 	"Middle";
const char* KeyBindings::sNone = 	"None";

