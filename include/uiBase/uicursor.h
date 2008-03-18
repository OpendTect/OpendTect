#ifndef uicursor_h
#define uicursor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.h,v 1.10 2008-03-18 17:38:05 cvskris Exp $
________________________________________________________________________

-*/

#include "mousecursor.h"
#include "thread.h"

class ioPixmap;
class QCursor;

class uiCursorManager : public MouseCursorManager
{
public:
    static void	initClass();

protected:
		~uiCursorManager();
		uiCursorManager();

    static void	fillQCursor(const MouseCursor&,QCursor&);
    void	setOverrideShape(MouseCursor::Shape,bool replace);
    void	setOverrideCursor(const MouseCursor&,bool replace);
    void	setOverrideFile(const char* filenm,
	    			int hotx,int hoty, bool replace);
    void	restoreInternal();
};

#endif
