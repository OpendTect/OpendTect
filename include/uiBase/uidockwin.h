#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.14 2007-02-28 07:29:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uihandle.h"

#ifdef USEQT3
# define mQDockWindow QDockWindow
#else
# define mQDockWindow QDockWidget
#endif

class uiDockWinBody;
class uiObject;
class uiGroup;
class mQDockWindow;

class uiDockWin : public uiParent
{
public:
			uiDockWin(uiParent* parnt=0, 
				  const char* nm="uiDockWin");
    virtual		~uiDockWin();

    void		setDockName(const char*);
    const char*		getDockName() const;

    uiGroup* 		topGroup();

    void		setResizeEnabled(bool);
    bool		isResizeEnabled() const;

    void		setFloating(bool);
    bool		isFloating() const;

    mQDockWindow*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();
};

#endif
