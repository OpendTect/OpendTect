#ifndef uicursor_h
#define uicursor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.h,v 1.11 2008-03-18 17:41:01 cvskris Exp $
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

    static void	fillQCursor(const MouseCursor&,QCursor&);

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
