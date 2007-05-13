#ifndef uimenu_h
#define uimenu_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.34 2007-05-13 15:42:36 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uihandle.h"
#include "separstr.h"

class uiParent;

class uiMenuItem;
class uiPopupMenu;
class uiPopupItem;

class uiMenuItemContainerBody;
class i_MenuMessenger;

#ifdef USEQT3
# define mQPopupMenu QPopupMenu
#else
template<class> class uiMenuItemContainerBodyImpl;
# define mQPopupMenu Q3PopupMenu
#endif

class QMenuBar;
class mQPopupMenu;
class QEvent;

template<class T> class ObjectSet;


class MenuItemSeparString : public SeparString
{
public:
    	MenuItemSeparString(const char* str=0) : SeparString(str,'`')	{}
};


class uiMenuItemContainer : public uiObjHandle
{
#ifdef USEQT3
friend class			uiMenuItemContainerBody;
protected:
				uiMenuItemContainer(const char* nm,
					    uiMenuItemContainerBody*);
#else
template<class> friend class	uiMenuItemContainerBodyImpl;
protected:
				uiMenuItemContainer(const char*,uiBody*,
					    uiMenuItemContainerBody*);
#endif

public:
				~uiMenuItemContainer();

    int				nrItems() const;
    const ObjectSet<uiMenuItem>& items() const;

    uiMenuItem*			find(const MenuItemSeparString&);
    uiMenuItem*			find(const char* itmtxt);
	
    int				insertItem(uiMenuItem*,int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    int				insertItem(const char* text, const CallBack& cb,
					   int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    int				insertItem(uiPopupMenu*,int id=-1,int idx=-1);
    				/*!<\param id The id that is returned if the
				  	      item is selected
				    \param idx pecifies the position in the
				    	       menu. The menu item is appended
					       at the end of the list if
					       negative.
			        */
    void			insertSeparator(int idx=-1);

    int				idAt(int idx) const;
    int				indexOf(int id) const;
    void			clear();

protected:
#ifdef USEQT3
    void			setMenuBody(uiMenuItemContainerBody*);
#endif
    uiMenuItemContainerBody*	body_;

};

/*! 

    Stores the id of the item in Qt and has a 
    messenger, so Qt's signals can be relayed.

*/
class uiMenuItem : public NamedObject
{
#ifdef USEQT3
friend class			uiMenuItemContainerBody;
#else
template<class> friend class	uiMenuItemContainerBodyImpl;
#endif

public:
				uiMenuItem(const char* nm);
				uiMenuItem(const char* nm,const CallBack& cb);
				~uiMenuItem();

				//! sets a new text 2b displayed
    void			setText(const char*);

    bool			isEnabled() const;
    void			setEnabled(bool);
    				/*!<\note Should be set after object is given
				          to it's parent, since parent will
					  overwrite this setting. */

    virtual bool		isCheckable() const	{ return checkable_; }
    virtual void		setCheckable( bool yn ) { checkable_ = yn; }

    bool			isChecked() const;
    void 			setChecked(bool);
    				/*!<\note Should be set after object is given
				          to it's parent, since parent will
					  overwrite this setting. */
    
    Notifier<uiMenuItem>	activated;

    				//! force activation in GUI thread
    void			activate();	
    Notifier<uiMenuItem>	activatedone;
    bool			handleEvent(const QEvent*);

    int				id() const		{ return id_; }
    int				index() const;

protected:

    void 			setId( int newid )	{ id_ = newid; }
    void			setMenu( uiMenuItemContainerBody* mb )
				{ menu_ = mb; }

    i_MenuMessenger*		messenger()		{ return &messenger_; }
    uiMenuItemContainerBody*	menu_;

private:

    i_MenuMessenger&            messenger_;

    int                         id_;
    bool			checkable_;
    bool			checked_;
    bool			enabled_;

};


class uiPopupItem : public uiMenuItem
{
friend class uiPopupMenu;
protected:
                                uiPopupItem(uiPopupMenu&,const char* nm);
public:

    virtual bool		isCheckable() const;
    virtual void		setCheckable(bool);

    uiPopupMenu&		menu()		{ return *popmenu_; }
    const uiPopupMenu&		menu() const	{ return *popmenu_; }
    
protected:

    uiPopupMenu*		popmenu_;

};


class QPixmap;

class uiMenuBar : public uiMenuItemContainer
{

    friend class		uiMainWinBody;
    friend class		uiDialogBody;

public:

    void			setIcon(const QPixmap&);
    void			setSensitive(bool yn);
    				/*!< Works on complete menubar */

protected:
				uiMenuBar(uiParent*,const char* nm);
				uiMenuBar(uiParent*,const char* nm,QMenuBar&);

    void 			reDraw(bool deep=true);

};


class uiPopupMenu : public uiMenuItemContainer
{

public:                        
				uiPopupMenu(uiParent* parnt,
					    const char* nm="uiPopupMenu");
				uiPopupMenu(uiParent* parnt,
					    mQPopupMenu* qmnu,
					    const char* nm="uiPopupMenu");

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

    uiPopupItem&		item_;
};

#endif
