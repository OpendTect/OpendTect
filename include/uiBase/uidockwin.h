#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiparent.h"

class uiDockWinBody;
class uiGroup;
class uiObject;
class uiMainWin;
mFDQtclass(QDockWidget)

mClass(uiBase) uiDockWin : public uiParent
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

    void		setMinimumWidth(int);

    mQtclass(QDockWidget*)	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();

    uiParent *		parent_;
};

#endif

