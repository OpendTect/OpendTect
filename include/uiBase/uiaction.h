#ifndef uiaction_h
#define uiaction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "separstr.h"
#include "uistring.h"
#include "uibaseobject.h"


class ioPixmap;
class MenuItem;
class uiMenu;
class uiActionContainer;
class i_ActionMessenger;
mFDQtclass(QAction);
mFDQtclass(QMenu);

/*!Represents either a menu item, or a toolbar item that can be
   clicked by a user .*/

mExpClass(uiBase) uiAction : public CallBacker
{
friend class		i_ActionMessenger;
public:
			uiAction(const uiString&);
			uiAction(const uiString&,const CallBack&);
			uiAction(const uiString&,const CallBack&,
				 const ioPixmap&);
			uiAction(const uiString&,const CallBack&,
				 const char* pmfln);
			uiAction(const uiString&,const char* pmfln);
			uiAction(const MenuItem&);
			uiAction(mQtclass(QAction*));
			~uiAction();

    void		setText(const uiString&);
    const uiString&	text() const;
			/*!<\note Use before next call.*/
    void		setIconText(const uiString&);
    const uiString&	iconText() const;
			/*!<\note Use before next call.*/
    void		setToolTip(const uiString&);
    const uiString&	toolTip() const;
			/*!<\note Use before next call.*/

    static void		updateToolTips();
    void		setMenu(uiMenu*);
			//!<Becomes mine

    uiMenu*		getMenu()		{ return menu_; }
    const uiMenu*	getMenu() const		{ return menu_; }

    void		setShortcut(const char*);

    void		setIcon(const ioPixmap&);
    void		setIcon(const char*);

    void		setCheckable(bool);
    bool		isCheckable() const;
    void		setChecked(bool);
    bool		isChecked() const;
    void		setEnabled(bool);
    bool		isEnabled() const;
    void		setVisible(bool);
    bool		isVisible() const;

    void		setSeparator(bool);
    bool		isSeparator() const;

    void		setParentContainer(const uiActionContainer*);
    int			getID() const;
			/*!<Only works if parent is set. */

    mQtclass(QAction*)	qaction()		{ return qaction_; }
    const mQtclass(QAction*)	qaction() const { return qaction_; }

    Notifier<uiAction>	toggled;
    Notifier<uiAction>	triggered;

    void		reloadIcon();

protected:

    virtual void	trigger(bool checked);
    void		translateCB(CallBacker*);

    void		updateToolTip();
    uiString		tooltip_;
    uiString		text_;
    uiString		icontext_;

private:

    uiMenu*			menu_;

    BufferString		iconfile_;
    const uiActionContainer*	parentcontainer_;
    i_ActionMessenger*		msgr_;
    mQtclass(QAction*)		qaction_;

    bool			checked_;

    int				cmdrecrefnr_;

    void			init(const uiString&);

public:
    //! Not for casual use
    static void         addCmdRecorder(const CallBack&);
    static void         removeCmdRecorder(const CallBack&);
    int  /* refnr */    beginCmdRecEvent(const char* msg=0);
    void                endCmdRecEvent(int refnr,const char* msg=0);

			//Legacy, to be removed
    void		setPixmap(const char* file) { setIcon(file); }
    void		setPixmap(const ioPixmap& p) { setIcon(p); }
};

/*!Represents a series of menu selections, from the top of a
 uiMenu (or uiToolBar) down to an item. */

mExpClass(uiBase) uiActionSeparString : public SeparString
{
public:
    uiActionSeparString(const char* str=0) : SeparString(str,'`')	{}
};


/*Generalization of a menubar, a menu or a toolbar that can contain actions. */
mExpClass(uiBase) uiActionContainer
{
public:
    virtual			~uiActionContainer();

    int				nrActions() const;
    const ObjectSet<uiAction>&	actions() const;

    uiAction*			findAction(const uiActionSeparString&);
    uiAction*			findAction(const char* itmtxt);
    uiAction*			findAction(int id);
    uiAction*			findAction(const uiMenu*);
    uiAction*			findAction( const FixedString& fs )
					{ return findAction(fs.str()); }
    int				getID(const uiAction*) const;
    int				getID(const mQtclass(QAction)*) const;

    int				insertAction(uiAction*,int id=-1,
					     const uiAction* before = 0);
				/*!<\param uiAction* becomes mine.
				    \param id The ID that is returned if the
					      item is selected.
				 */

    uiMenu*			addMenu(uiMenu*,const uiMenu* before = 0);
				/*!<Becomes mine. Returns pointer to menu. */

    uiAction*			insertSeparator();

    void			removeAction(uiAction*);
    void			removeAction(int id);
    void			removeAllActions();
    void			reloadIcons();

protected:
			uiActionContainer();

    int			getFreeID() const;

    virtual void	doInsertSeparator(mQtclass(QAction)* before)	= 0;
    virtual void	doInsertMenu(mQtclass(QMenu)*,
				 mQtclass(QAction)* before )		= 0;
    virtual void	doInsertAction(mQtclass(QAction)*,
				       mQtclass(QAction)* before )	= 0;
    virtual void	doClear()					= 0;
    virtual void	doRemoveAction(mQtclass(QAction)*)		= 0;

    ObjectSet<uiAction>	actions_;
    TypeSet<int>	ids_;

public:

    int			insertItem(uiAction* a,int id=-1,
				   const uiAction* before = 0)
			{ return insertAction( a, id, before ); }
			//!<Legacy
    int			insertItem(uiMenu* mnu);
			/*!<Legacy. */
    void		removeItem(uiAction* a) { removeAction(a); }
			//!<Legacy
    void		removeItem(int id) { removeAction(id); }
			//!<Legacy
};

#endif
