#ifndef uimenu_h
#define uimenu_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.60 2012/09/17 16:37:45 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uibaseobject.h"
#include "uiicons.h"
#include "separstr.h"


class uiMenuItem;
class uiMenuItemContainerBody;
class uiParent;
class uiPopupMenu;
class uiPopupItem;
class ioPixmap;

class i_MenuMessenger;
template<class> class uiMenuItemContainerBodyImpl;

class QAction;
class QMenu;
class QMenuBar;

template<class T> class ObjectSet;


mClass MenuItemSeparString : public SeparString
{
public:
    	MenuItemSeparString(const char* str=0) : SeparString(str,'`')	{}
};


mClass uiMenuItemContainer : public uiBaseObject
{
template<class> friend class	uiMenuItemContainerBodyImpl;

public:
				~uiMenuItemContainer();

    int				nrItems() const;
    const ObjectSet<uiMenuItem>& items() const;

    uiMenuItem*			find(const MenuItemSeparString&);
    uiMenuItem*			find(const char* itmtxt);
    uiMenuItem*			find(int id);
	
    int				insertItem(uiMenuItem*,int id=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
			        */
    int				insertItem(uiPopupMenu*,int id=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
			        */
    int				insertMenu(uiPopupMenu*,uiPopupMenu* before);
    void			insertSeparator(int) { insertSeparator(); }
    void			insertSeparator();

    void			removeItem(uiMenuItem*);
    void			removeItem(int id,bool withdelete=true);
    void			clear();

    void			translate();

protected:
				uiMenuItemContainer(const char*,uiBody*,
					    uiMenuItemContainerBody*);

    uiMenuItemContainerBody*	body_;
};


/*! 
    Stores the id of the item in Qt and has a 
    messenger, so Qt's signals can be relayed.
*/

mClass uiMenuItem : public NamedObject
{
template<class> friend class	uiMenuItemContainerBodyImpl;

public:
				uiMenuItem(const char* nm,const char* pmnm=0);
				uiMenuItem(const char* nm,const CallBack& cb,
					   const char* pixmapnm=0);
				~uiMenuItem();
    int				id() const		{ return id_; }

    const QAction*		qAction() const		{ return qaction_; }

				//! sets a new text 2b displayed
    void			setText(const char*);
    void			setText(const wchar_t*);
    const char*			text() const;

    void			setPixmap(const char*);
    void			setPixmap(const ioPixmap&);
    void			setShortcut(const char*);

    bool			isEnabled() const;
    void			setEnabled(bool);
    				/*!<\note Should be set after object is given
				          to it's parent, since parent will
					  overwrite this setting. */

    bool			isCheckable() const;
    void			setCheckable(bool);

    bool			isChecked() const;
    void 			setChecked(bool);
    				/*!<\note Should be set after object is given
				          to it's parent, since parent will
					  overwrite this setting. */

    void			translate();
    
    Notifier<uiMenuItem>	activated;

protected:

    void 			setId( int newid )	{ id_ = newid; }
    void			setMenu( uiMenuItemContainerBody* mb )
				{ menu_ = mb; }
    void			setAction( QAction* act ) { qaction_ = act; }

    i_MenuMessenger*		messenger()		{ return &messenger_; }
    uiMenuItemContainerBody*	menu_;
    QAction*			qaction_;

    void			trlReady(CallBacker*);
    int				translateid_;

private:

    i_MenuMessenger&            messenger_;

    int                         id_;
    bool			checkable_;
    bool			checked_;
    bool			enabled_;
    const ioPixmap*		pixmap_;

    static CallBack*		cmdrecorder_;		// obsolete
    int				cmdrecrefnr_;

public:
				//! Not for casual use
    static void			setCmdRecorder(const CallBack&); // obsolete
    static void			unsetCmdRecorder();		 // obsolete
    int  /* refnr */		beginCmdRecEvent(const char* msg=0);
    void			endCmdRecEvent(int refnr,const char* msg=0);

    static void			addCmdRecorder(const CallBack&);
    static void			removeCmdRecorder(const CallBack&);
};


mClass uiPopupItem : public uiMenuItem
{
friend class uiPopupMenu;
protected:
                                uiPopupItem( uiPopupMenu& pm, const char* nm,
				       	     const char* pixmapfnm=0 )
				    : uiMenuItem(nm,pixmapfnm)
				    , popmenu_(&pm)	{}
				//!<pixmap must be alive in memory until
				//!<item is added to parent
public:

    uiPopupMenu&		menu()		{ return *popmenu_; }
    const uiPopupMenu&		menu() const	{ return *popmenu_; }
    
protected:

    uiPopupMenu*		popmenu_;

};


class QPixmap;

mClass uiMenuBar : public uiMenuItemContainer
{

    friend class		uiMainWinBody;
    friend class		uiDialogBody;

public:

    void			setIcon(const QPixmap&);
    void			setSensitive(bool yn);
    				/*!< Works on complete menubar */
    bool			isSensitive() const;

protected:
				uiMenuBar(uiParent*,const char* nm);
				uiMenuBar(uiParent*,const char* nm,QMenuBar&);

    void 			reDraw(bool deep=true);

};


mClass uiPopupMenu : public uiMenuItemContainer
{

public:                        
				uiPopupMenu(uiParent*,
					    const char* nm="uiPopupMenu",
					    const char* pixmapfilenm=0);
				//!<pixmap must be alive in memory until
				//!<item is added to parent

				~uiPopupMenu();

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

    uiPopupItem&		item()			{ return item_; }
    const uiPopupItem&		item() const		{ return item_; }

private:

    int				findIdForAction(QAction*) const;
    uiPopupItem&		item_;

    static CallBack*		interceptor_;		// obsolete
    uiMenuItem*			interceptitem_;
    bool			dointercept_;

    ioPixmap*			pixmap_;

public:
				//! Not for casual use
    static void			setInterceptor(const CallBack&); // obsolete
    static void			unsetInterceptor();		 // obsolete
    void			doIntercept(bool yn,uiMenuItem* activateitm=0);

    static void			addInterceptor(const CallBack&);
    static void			removeInterceptor(const CallBack&);
};

#endif
