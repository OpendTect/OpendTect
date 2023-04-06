#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
{ mODTextTranslationClass(uiDockWin)
public:
			uiDockWin(uiParent* parnt=0,
			    const uiString& caption=uiString::emptyString() );
			~uiDockWin();
			mOD_DisableCopy(uiDockWin)

    void		setGroup(uiGroup*);
    void		setObject(uiObject*);
    void		setVisible(bool yn);
    bool		isVisible() const;

    void		setDockName(const uiString&);
    uiString		getDockName() const;

    uiGroup* 		topGroup();
    const uiGroup*	topGroup() const
			    { return const_cast<uiDockWin*>(this)->topGroup(); }

    uiMainWin*		mainwin() override;

    void		setFloating(bool);
    bool		isFloating() const;

    void		setMinimumWidth(int);

    mQtclass(QDockWidget*)	qwidget();

protected:

    uiDockWinBody*	body_;
    uiObject*		mainobject() override;

    uiParent *		parent_;
};
