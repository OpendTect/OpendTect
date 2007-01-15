#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.11 2007-01-15 15:20:56 cvsnanne Exp $
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
    void		setDockName(const char*);
    const char*		getDockName() const;

    uiGroup* 		topGroup();

    void		setHorStretchable(bool);
    bool		isHorStretchable() const;

    void		setVerStretchable(bool);
    bool		isVerStretchable() const;

    void		setResizeEnabled(bool);
    bool		isResizeEnabled() const;

    void		setCloseMode(int);
			/*!< 0: Never; 1: Docked; 2: Undocked; 3: Always */
    int			closeMode() const;

    mQDockWindow*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();
};

#endif
