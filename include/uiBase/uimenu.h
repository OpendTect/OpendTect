#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiaction.h"
#include "uibaseobject.h"
#include "separstr.h"

class uiParent;
class uiPixmap;
class uiButton;
class uiToolBar;

mFDQtclass(QAction)
mFDQtclass(QMenu)
mFDQtclass(QMenuBar)



mExpClass(uiBase) uiMenuBar : public uiActionContainer, public uiBaseObject
{
    friend class		uiMainWinBody;
    friend class		uiDialogBody;

public:

    void			setSensitive(bool yn);
				/*!< Works on complete menubar */
    bool			isSensitive() const;

    int				getNrWidgets() const	{ return 1; }
    mQtclass(QWidget*)		getWidget(int);
    void			clear() { removeAllActions(); }

protected:
				uiMenuBar(uiParent*,const char* nm);
				uiMenuBar(uiParent*,const char* nm,
					  mQtclass(QMenuBar)*);
				~uiMenuBar();

    void			doInsertMenu(mQtclass(QMenu)*,
				   mQtclass(QAction)* before);
    void			doInsertAction(mQtclass(QAction)*,
				     mQtclass(QAction)* before);
    void			doRemoveAction(mQtclass(QAction)*);
    void			doInsertSeparator(mQtclass(QAction)* before);
    void			doClear();

    mQtclass(QMenuBar)*		qmenubar_;
};


/*!A standard menu. If you create one yourself, you own it and should
   delete it. If you insert it into a menu or toolbar, ownership
   is transfered. */

mExpClass(uiBase) uiMenu : public uiActionContainer, public uiBaseObject
{ mODTextTranslationClass(uiMenu);

public:

			uiMenu(const uiString& txt=uiString::empty(),
			       const char* icnm=0);
			uiMenu(const MenuItem&);
			~uiMenu();

    uiMenu*		addSubMenu(uiParent*,const uiString&,const char* icnm);
    void		addItems(const ObjectSet<MenuItem>&);
    bool		isStandAlone() const { return !submenuaction_; }
			/*!\returns true if this menu is not
			    inserted into a menu with an item that
			    owns it. */

    bool		isCheckable() const;
    void		setCheckable(bool);

    bool		isEnabled() const;
    void		setEnabled(bool);

    bool		isChecked() const;
    void		setChecked(bool);

			/*! \brief pops-up at mouse position

			    The return code is the id of the selected
			    item in either the popup menu or one of
			    its submenus, or -1 if no item is selected
			    (normally because the user presses Escape).
			*/
    int			exec();

    void		setText(const uiString&);
    const uiString&	text() const;
    bool		isEmpty() const;
    void		clear() { removeAllActions(); }

    void		setIcon(const uiIcon&);
    void		setIcon(const char* iconnm);
    const char*		getIconName() const;

    mQtclass(QMenu)*	getQMenu() { return qmenu_; }
    int			getNrWidgets() const	{ return 1; }
    mQtclass(QWidget*)	getWidget(int);

private:

    void		setAction(uiAction*);

    uiString		text_;
    BufferString	iconnm_;

    mQtclass(QMenu)*	qmenu_;
    uiAction*		submenuaction_;
    uiAction*		interceptaction_;
    bool		dointercept_;

    void		doInsertMenu(mQtclass(QMenu)*,
				   mQtclass(QAction)* before);
    void		doInsertAction(mQtclass(QAction)*,
				     mQtclass(QAction)* before);
    void		doInsertSeparator(mQtclass(QAction)* before);
    void		doRemoveAction(mQtclass(QAction)*);
    void		doClear();
    void		useStyleSheet();

    friend class	uiAction;
    friend class	uiButton;
    friend class	uiToolBar;

public:
			//! Not for casual use
    static void		addInterceptor(const CallBack&);
    static void		removeInterceptor(const CallBack&);
    void		doIntercept(bool yn,uiAction* activateitm = 0);

			uiMenu(uiParent*,
				const uiString& txt=uiString::empty(),
				const char* iconfilenm=0);
};

