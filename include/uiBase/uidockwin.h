#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.9 2005-11-18 15:11:34 cvsarend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

#ifdef USEQT4
# define mQDockWindow Q3DockWindow
#else
# define mQDockWindow QDockWindow
#endif

class uiDockWinBody;
class uiObject;
class uiGroup;
class mQDockWindow;

class uiDockWin : public uiParent
{

public:
			uiDockWin( uiParent* parnt=0, 
				   const char* nm="uiDockWin" );

    virtual		~uiDockWin();


    uiGroup* 		topGroup();

    void		setHorStretchable(bool);
    bool		isHorStretchable() const;

    void		setVerStretchable(bool);
    bool		isVerStretchable() const;

    void		setResizeEnabled(bool);
    bool		isResizeEnabled() const;

    mQDockWindow*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();
};

#endif
