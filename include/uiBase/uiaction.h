#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "separstr.h"
#include "uistring.h"
#include "uibaseobject.h"


class i_ActionMessenger;
class uiActionContainer;
class uiIcon;
class uiMenu;
class uiPixmap;
class MenuItem;

mFDQtclass(QAction)
mFDQtclass(QMenu)


/*!Represents either a menu item, or a toolbar item that can be
   clicked by a user .*/

mExpClass(uiBase) uiAction : public CallBacker
{
friend class		i_ActionMessenger;
public:
			uiAction(const uiString&);
			uiAction(const uiString&,const CallBack&);
			uiAction(const uiString&,const CallBack&,
				 const uiIcon&);
			uiAction(const uiString&,const CallBack&,
				 const char* icon_identifier);
			uiAction(const uiString&,const char* icon_identifier);
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
    const uiActionContainer* getContainer() const;

    void		setShortcut(const char*);

    void		setIcon(const char* identifier);
    void		setIcon(const uiIcon&);
    const char*		getIconName() const		{ return iconfile_; }
    void		setPixmap(const uiPixmap&);

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

    mQtclass(QAction*)		qaction()	{ return qaction_; }
    const mQtclass(QAction*)	qaction() const { return qaction_; }

    Notifier<uiAction>	toggled;
    Notifier<uiAction>	triggered;

    void		reloadIcon();

protected:

    virtual void	trigger(bool checked);
    void		translateCB(CallBacker*);

    void		updateToolTip(CallBacker* = 0);
    uiString		tooltip_;
    uiString		text_;
    uiString		icontext_;

    void		updateEnabled(CallBacker* = 0);
    void		updateCheckable(CallBacker* = 0);
    void		updateChecked(CallBacker* = 0);
    void		updateVisible(CallBacker* = 0);
    bool		enabled_;
    bool		checkable_;
    bool		ischecked_;
    bool		visible_;

private:
				uiAction();

    uiMenu*			menu_ = nullptr;

    BufferString		iconfile_;
    const uiActionContainer*	parentcontainer_ = nullptr;
    i_ActionMessenger*		msgr_ = nullptr;
    mQtclass(QAction*)		qaction_;

    bool			checked_;

    int				cmdrecrefnr_ = 0;

    void			init(const uiString&);

public:

    //! Not for casual use
    static void         addCmdRecorder(const CallBack&);
    static void         removeCmdRecorder(const CallBack&);
    int  /* refnr */    beginCmdRecEvent(const char* msg=0);
    int  /* refnr */	beginCmdRecEvent(const BufferString& msg);
    void                endCmdRecEvent(int refnr,const char* msg=0);
};

/*!Represents a series of menu selections, from the top of a
 uiMenu (or uiToolBar) down to an item. */

mExpClass(uiBase) uiActionSeparString : public SeparString
{
public:
				uiActionSeparString(const char* str=nullptr);
    virtual			~uiActionSeparString();
};


/*Generalization of a menubar, a menu or a toolbar that can contain actions. */
mExpClass(uiBase) uiActionContainer
{
public:
    virtual			~uiActionContainer();

    int				nrActions() const;
    const ObjectSet<uiAction>&	actions() const;
    bool			isEmpty() const;

    const uiAction*		findAction(const uiActionSeparString&) const;
    const uiAction*		findAction(const char* itmtxt) const;
    const uiAction*		findAction(const uiString& itmtxt) const;
				//!<The full string of the text will be used
    const uiAction*		findAction(int id) const;
    const uiAction*		findAction(const uiMenu*) const;
    const uiAction*		findAction( const StringView& fs ) const
					{ return findAction(fs.str()); }
    int				getID(const uiAction*) const;
    int				getID(const mQtclass(QAction)*) const;

    int				insertAction(uiAction* act,int id=-1,
					     const uiAction* before = 0);
				/*!<\param act becomes mine.
				    \param id The ID that is returned if the
					      item is selected.
				    \param before Insert action before this one
				 */
    int				insertAction(const MenuItem&);

    bool			isMenu() const;
    uiMenu*			addMenu(uiMenu*,const uiAction* before);
				/*!<Becomes mine. Returns pointer to menu. */
    uiMenu*			addMenu(uiMenu*,const uiMenu* before = 0);
				/*!<Becomes mine. Returns pointer to menu. */

    uiAction*			insertSeparator();

    void			removeAction(uiAction*);
    void			removeAction(int id);
    bool			removeMenu(uiMenu&);
    void			removeAllActions();
    void			reloadIcons();

    void			shareActionsFrom(const uiActionContainer*);
    bool			hasSharedActions() const;

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
    bool		hassharedactions_		= false;

public:

    mDeprecated		("Use insertAction")
    int			insertItem(uiAction* a,int id=-1,
				   const uiAction* before = 0)
			{ return insertAction( a, id, before ); }
    mDeprecated		("Use addMenu")
    int			insertItem(uiMenu* mnu);
    void		removeItem(uiAction* a) { removeAction(a); }
			//!<Legacy
    void		removeItem(int id) { removeAction(id); }
			//!<Legacy
};
