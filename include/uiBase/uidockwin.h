#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.16 2007-12-24 05:29:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uihandle.h"

class uiDockWinBody;
class uiGroup;
class uiObject;
class QDockWidget;

class uiDockWin : public uiParent
{
public:
			uiDockWin(uiParent* parnt=0, 
				  const char* nm="uiDockWin");
    virtual		~uiDockWin();

    void		setDockName(const char*);
    const char*		getDockName() const;

    uiGroup* 		topGroup();
    const uiGroup* 	topGroup() const 
			    { return const_cast<uiDockWin*>(this)->topGroup(); }

    void		setFloating(bool);
    bool		isFloating() const;

    QDockWidget*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();
};

#endif
