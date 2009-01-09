#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.19 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"

class uiDockWinBody;
class uiGroup;
class uiObject;
class uiMainWin;
class QDockWidget;

mClass uiDockWin : public uiParent
{
public:
			uiDockWin(uiParent* parnt=0, 
				  const char* nm="uiDockWin");
    virtual		~uiDockWin();

    void		setGroup(uiGroup*);
    void		setObject(uiObject*);

    void		setDockName(const char*);
    const char*		getDockName() const;

    uiGroup* 		topGroup();
    const uiGroup* 	topGroup() const 
			    { return const_cast<uiDockWin*>(this)->topGroup(); }

    virtual uiMainWin*	mainwin();

    void		setFloating(bool);
    bool		isFloating() const;

    QDockWidget*	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();

    uiParent *		parent_;
};

#endif
