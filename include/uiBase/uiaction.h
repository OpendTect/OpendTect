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
#include "uibaseobject.h"


class ioPixmap;
class uiMenu;
class MenuItem;
class uiActionContainer;
class i_ActionMessenger;
mFDQtclass(QAction);
mFDQtclass(QMenu);
mFDQtclass(QString);

/*!Represents either a menu item, or a toolbar item that can be
   clicked by a user .*/

mExpClass(uiBase) uiAction : public CallBacker
{
friend class		i_ActionMessenger;
public:
                        uiAction(const char*);
                        uiAction(const char*,const CallBack&);
                        uiAction(const char*,const CallBack&,const ioPixmap&);
    			uiAction(const char*,const CallBack&,const char* pmfln);
    			uiAction(const char*,const char* pmfln);
    			uiAction(const MenuItem&);
			uiAction(mQtclass(QAction*));
			~uiAction();

    void		setText(const char*);
    void		setText(const wchar_t* txt);
    const char*		text() const;
    			/*!<\note Use before next call.*/
    void		setIconText(const char*);
    const char*		iconText() const;
    			/*!<\note Use before next call.*/
    void		setToolTip(const char*);
    void		setToolTip(const wchar_t*);
    const char*		toolTip() const;
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
    
    void		setParentContainer(const uiActionContainer*);
    int			getID() const;
    			/*!<Only works if parent is set. */
    
    mQtclass(QAction*)	qaction()		{ return qaction_; }
    const mQtclass(QAction*)	qaction() const { return qaction_; }

    Notifier<uiAction>	toggled;
    Notifier<uiAction>	triggered;
    
    virtual void	translate();
    void		reloadIcon();
    
protected:

    virtual void	trigger(bool checked);
    void		translationReadyCB(CallBacker*);
    void		doTranslate();
    void		setToolTip(const mQtclass(QString&));

    void		updateToolTip();
    mQtclass(QString*)	qnormaltooltipstr_;
    mQtclass(QString*)	qtranslatedtooltipstr_;

private:

    uiMenu*			menu_;

    BufferString		iconfile_;
    const uiActionContainer*	parentcontainer_;
    i_ActionMessenger*		msgr_;
    mQtclass(QAction*)		qaction_;

    bool			checked_;
    int				translateid_;
    
    int				cmdrecrefnr_;

    void			init(const char*);
    
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
    virtual 			~uiActionContainer();
    
    int				nrActions() const;
    const ObjectSet<uiAction>&	actions() const;
    
    uiAction*			findAction(const uiActionSeparString&);
    uiAction*			findAction(const char* itmtxt);
    uiAction*			findAction(int id);
    uiAction*			findAction(const uiMenu*);
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
    
    void			insertSeparator();
    
    void			removeAction(uiAction*);
    void			removeAction(int id);
    void			removeAllActions();
    virtual void		translate();
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

