#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.6 2002-05-14 15:18:03 nanne Exp $
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

    uiObject*		uiObj();
    const uiObject*	uiObj() const;

    uiGroup* 		topGroup();

    void		setHorStretchable(bool);
    bool		isHorStretchable() const;

    void		setVerStretchable(bool);
    bool		isVerStretchable() const;

    void		setResizeEnabled(bool);
    bool		isResizeEnabled() const;

    void		display( bool yn = true, bool shrink=false );

    QDockWindow*	qwidget();

protected:

    uiDockWinBody*	body_;
};

#endif
