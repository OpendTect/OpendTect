#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.8 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiDockWinBody;
class uiObject;
class uiGroup;
class QDockWindow;

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

    QDockWindow*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();
};

#endif
