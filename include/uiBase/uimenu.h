#ifndef uiMenu_H
#define uiMenu_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.h,v 1.3 2001-05-16 14:58:45 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uidobjset.h>

class QMenuData;
class QMenuBar;
class QPopupMenu;

template <class T> class i_QObjWrapper;
mTemplTypeDefT( i_QObjWrapper, QMenuData,  i_QMenuData )
mTemplTypeDefT( i_QObjWrapper, QPopupMenu, i_QPopupMenu )

class uiMenuItem;
class i_MenuMessenger;
class uiPopupMenu;
class uiMenuItem;

class QObject;


class uiMenuData  
{
public:

    virtual		~uiMenuData();
    
    void		insertItem(uiMenuItem*,int idx=-1);
    void                insertItem(const char* text, const CallBack& cb, 
				   int idx=-1);
    void		insertItem(uiPopupMenu*,int idx=-1);

    void 		insertSeparator(int idx=-1);

    virtual QMenuData& 	qMenuData() = 0;

protected:
			uiMenuData();

    UserIDObjectSet<UserIDObject> itms;

};


class uiMenuItem : public UserIDObject
{
    friend class	uiMenuData;
    friend class	i_MenuMessenger;

public:
			uiMenuItem( const char* nm="uiMenuItem" );
			uiMenuItem( const char* nm, const CallBack& cb ); 

    virtual		~uiMenuItem();

			//! sets a new text 2b displayed
    void		set( const char* txt );

    void                notify( const CallBack& cb ) { notifyCBL += cb; }

    bool 		isEnabled() 			const;
    void 		setEnabled( bool yn );
    bool 		isChecked()			const;
    void 		setChecked( bool yn );

protected:

    //! Handler called from Qt.
    virtual void        notifyHandler() 	{ Notifier(); }

    void                Notifier()     { notifyCBL.doCall(this); }
    CallBackList        notifyCBL;

    i_MenuMessenger&	_messenger;
    int			id;
    uiMenuData*		parent;

};

class uiMenuBar : public uiMenuData, public uiNoWrapObj<QMenuBar>
{
    friend class	uiMainWin;

public:                        

    virtual QMenuData& 	qMenuData();

protected:
			uiMenuBar( uiMainWin* parnt, const char* nm, 
				   QMenuBar& qThing );
    const QWidget*	qWidget_() const;

};


class uiPopupMenu : public uiMenuData, public uiWrapObj<i_QPopupMenu>
{
    friend class	uiMenuData;

public:                        
			uiPopupMenu( uiObject* parnt,
                                     const char* nm="uiPopupMenu");

    virtual QMenuData& 	qMenuData();

    bool		isCheckable();
    void		setCheckable( bool yn );

protected:

    const QWidget*	qWidget_() const;

    int			id;

};

#endif
