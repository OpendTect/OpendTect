#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/01/2002
 RCS:           $Id: uitabbar.h,v 1.1 2002-01-16 11:01:23 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class ioPixmap;
class uiTabBarBody;
class PtrUserIDObjectSet;

class uiTabBar : public uiObject
{
public:
			uiTabBar( uiParent*, const char* nm,
				  const CallBack* cb =0 );

			//! returns id of  new tab
    int			addTab( const char* );
    int			addTab( const ioPixmap& );
    int			insertTab( const char*, int index = -1 );
    int			insertTab( const ioPixmap&, int index = -1 );
    void		removeTab( int id );

    void		addTabs( const char** textList );
    void 		addTabs( const PtrUserIDObjectSet& uids );
    void 		addTabs( const ObjectSet<BufferString>& strs );

    void		setTabEnabled( int id, bool );
    bool		isTabEnabled( int id ) const;

    int			currentTab() const;
    int			keyboardFocusTab() const;

    int			indexOf( int id ) const;
    int			size() const;

    void		removeToolTip( int id );
    void		setToolTip( int id, const char* tip );
    const char*		toolTip( int id ) const;

    void		setCurrentTab( int id );
    void		setCurrentTab( const char* );

    void		setTabText( int id, const char* txt );

    Notifier<uiTabBar>  selected;

protected:

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*, const char*);

    mutable BufferString	rettxt;

};

#endif
