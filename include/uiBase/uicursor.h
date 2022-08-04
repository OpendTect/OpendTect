#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"
#include "mousecursor.h"
#include "thread.h"

mFDQtclass(QCursor)

mExpClass(uiBase) uiCursorManager : public MouseCursorManager
{
public:
    static void	initClass();

    static uiPoint cursorPos();

    static void	fillQCursor(const MouseCursor&,mQtclass(QCursor&));

    static void setPriorityCursor(MouseCursor::Shape);
    static void unsetPriorityCursor();

    static MouseCursor::Shape overrideCursorShape();

protected:
		~uiCursorManager();
		uiCursorManager();

    void	setOverrideShape(MouseCursor::Shape,bool replace) override;
    void	setOverrideCursor(const MouseCursor&,bool replace) override;
    void	setOverrideFile(const char* filenm,
				int hotx,int hoty, bool replace) override;
    void	restoreInternal() override;
};

