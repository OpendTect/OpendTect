#ifndef uiMenu_H
#define uiMenu_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.5 2001-08-24 14:23:42 arend Exp $
________________________________________________________________________

-*/

#include <uihandle.h>
#include <uidobjset.h>

class uiParent;
class uiMainWin;

class uiMenuItem;
class uiPopupMenu;

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
				uiMenuItem( const char* nm="uiMenuItem" );
				uiMenuItem( const char* nm, const CallBack& cb);
				~uiMenuItem();

				//! sets a new text 2b displayed
    void			set( const char* txt );

    bool			isEnabled()			const;
    void			setEnabled( bool yn );
    bool			isChecked()			const;
    void 			setChecked( bool yn );

    Notifier<uiMenuItem>	activated;

protected:

    void 			setId( int id )                 { id_ = id; }
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
				uiPopupItem( uiPopupMenu& menu,
					     const char* nm="uiMenuItem");
public:

    bool			isCheckable();
    void			setCheckable( bool yn );

private:

    //uiPopupMenu*		popmenu_;

};


class uiMenuBar : public uiMenuData
{
    friend class		uiMainWinBody;
public:
				uiMenuBar( uiParent* parnt, const char* nm );
protected:
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

    uiPopupItem&		item()			{ return item_; }

private:

    uiPopupItem&		item_;

};

#endif
