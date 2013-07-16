#ifndef uimenu_h
#define uimenu_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiaction.h"
#include "uibaseobject.h"
#include "uiicons.h"
#include "separstr.h"

class uiParent;
class ioPixmap;

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
    
    mQtclass(QWidget*)		getWidget();
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
{

public:                        
				uiMenu(const char* nm="uiMenu",
				       const char* pixmapfilenm=0);

				~uiMenu();

    bool			isStandAlone() const { return !submenuaction_; }
    				/*!\returns true if this menu is not
				    inserted into a menu with an item that
				    owns it. */

    bool			isCheckable() const;
    void			setCheckable(bool);

    bool			isEnabled() const;
    void			setEnabled(bool);

    bool			isChecked() const;
    void			setChecked(bool);

				/*! \brief pops-up at mouse position

				    The return code is the id of the selected 
				    item in either the popup menu or one of 
				    its submenus, or -1 if no item is selected
				    (normally because the user presses Escape). 
				*/
    int				exec();
    
    void			setName(const char*);
    void			clear() { removeAllActions(); }
    
    mQtclass(QMenu)*		getQMenu() { return qmenu_; }
    mQtclass(QWidget*)		getWidget();

private:
    friend class		uiAction;

    void			setAction(uiAction*);

    mQtclass(QMenu)*		qmenu_;
    uiAction*			submenuaction_;
    uiAction*			interceptaction_;
    bool			dointercept_;
    
    void			doInsertMenu(mQtclass(QMenu)*,
					   mQtclass(QAction)* before);
    void			doInsertAction(mQtclass(QAction)*,
					     mQtclass(QAction)* before);
    void			doInsertSeparator(mQtclass(QAction)* before);
    void			doRemoveAction(mQtclass(QAction)*);
    void			doClear();

public:
				//! Not for casual use
    static void			addInterceptor(const CallBack&);
    static void			removeInterceptor(const CallBack&);
    void			doIntercept(bool yn,uiAction* activateitm = 0);

				uiMenu(uiParent*, const char* nm="uiMenu",
				       const char* pixmapfilenm=0);
};

#endif

