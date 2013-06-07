#ifndef uicursor_h
#define uicursor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigeom.h"
#include "mousecursor.h"
#include "thread.h"

class ioPixmap;
class QCursor;

mClass uiCursorManager : public MouseCursorManager
{
public:
    static void	initClass();

    static uiPoint cursorPos();

    static void	fillQCursor(const MouseCursor&,QCursor&);

    static void setPriorityCursor(MouseCursor::Shape);
    static void unsetPriorityCursor();

    static MouseCursor::Shape overrideCursorShape();

protected:
		~uiCursorManager();
		uiCursorManager();

    void	setOverrideShape(MouseCursor::Shape,bool replace);
    void	setOverrideCursor(const MouseCursor&,bool replace);
    void	setOverrideFile(const char* filenm,
	    			int hotx,int hoty, bool replace);
    void	restoreInternal();
};

#endif
