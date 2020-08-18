#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/02/2002
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiparent.h"

class uiDockWinBody;
class uiGroup;
class uiObject;
class uiMainWin;
mFDQtclass(QDockWidget)

mExpClass(uiBase) uiDockWin : public uiParent
{ mODTextTranslationClass(uiDockWin);
public:
			uiDockWin(uiParent* parnt=0,
                              const uiString& caption=uiString::empty() );

    virtual		~uiDockWin();

    void		setGroup(uiGroup*);
    void		setObject(uiObject*);

    void		setDockName(const uiString&);
    uiString		getDockName() const;

    uiGroup*		topGroup();
    const uiGroup*	topGroup() const
			    { return const_cast<uiDockWin*>(this)->topGroup(); }

    virtual uiMainWin*	mainwin();

    void		setFloating(bool);
    bool		isFloating() const;

    void		setMinimumWidth(int);

    int			getNrWidgets() const		{ return 1; }
    mQtclass(QDockWidget*)	qwidget();

protected:

    uiDockWinBody*	body_;
    virtual uiObject*	mainobject();

    uiParent *		parent_;
};
