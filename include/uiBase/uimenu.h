#ifndef uiMenu_H
#define uiMenu_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.12 2002-02-14 10:58:04 nanne Exp $
________________________________________________________________________

-*/

#include <uihandle.h>
#include <uidobjset.h>

class uiParent;
class uiMainWin;

class uiMenuItem;
class uiPopupMenu;
class uiPopupItem;

class uiMenuDataBody;
class i_MenuMessenger;


class QMenuBar;

class uiMenuData : public uiObjHandle
{
friend class			uiMenuDataBody;
protected:
				uiMenuData( const char* nm, uiMenuDataBody* b );
public:
				~uiMenuData();

    void			insertItem(uiMenuItem*,int idx=-1);
    void			insertItem(const char* text, const CallBack& cb,
					   int idx=-1);
    void			insertItem(uiPopupMenu*,int idx=-1);
    void			insertSeparator(int idx=-1);

protected:

    void			setMenuBody(uiMenuDataBody* b);
    uiMenuDataBody*		body_;

};

/*! 

    Stores the id of the item in Qt and has a 
    messenger, so Qt's signals can be relayed.

*/
class uiMenuItem : public UserIDObject
{
friend class			uiMenuDataBody;

public:
				uiMenuItem( const char* nm );
				uiMenuItem( const char* nm, const CallBack& cb);
				~uiMenuItem();

				//! sets a new text 2b displayed
    void			setText(const char*);

    bool			isEnabled()			const;
    void			setEnabled( bool yn );
    bool			isChecked()			const;
    void 			setChecked( bool yn );

    Notifier<uiMenuItem>	activated;

    int				id() const			{ return id_; }

protected:

    void 			setId( int i )                  { id_ = i; }
    void			setMenu( uiMenuDataBody* m )	{ menu_ = m; }

    i_MenuMessenger*		messenger()		{ return &messenger_; }
    uiMenuDataBody*             menu_;

private:

    i_MenuMessenger&            messenger_;

    int                         id_;

};


class uiPopupItem : public uiMenuItem
{
friend class uiPopupMenu;
protected:
                                uiPopupItem( uiPopupMenu& menu, const char* nm);
public:

    bool                        isCheckable();
    void                        setCheckable( bool yn );

};


class uiMenuBar : public uiMenuData
{
    friend class		uiMainWinBody;
    friend class		uiDialogBody;
protected:
				uiMenuBar( uiParent* parnt, const char* nm );
				uiMenuBar( uiMainWin* parnt, const char* nm, 
					   QMenuBar& );

    void 			reDraw(bool deep=true);

};

class uiPopupMenu : public uiMenuData
{

public:                        
				uiPopupMenu( uiParent* parnt,
					     const char* nm="uiPopupMenu");
				~uiPopupMenu();

    bool			isCheckable();
    void			setCheckable( bool yn );

				/*! \brief pops-up at mouse position

				    The return code is the id of the selected 
				    item in either the popup menu or one of 
				    its submenus, or -1 if no item is selected
				    (normally because the user presses Escape). 
				*/
    int				exec(); 

    uiPopupItem&		item()			{ return item_; }

private:

    uiPopupItem&		item_;

};

#endif
