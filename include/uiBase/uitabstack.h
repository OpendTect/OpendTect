#ifndef uitabstack_h
#define uitabstack_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabstack.h,v 1.1 2003-04-22 09:49:42 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>

class uiTabBar;

class uiTabStack : public uiGroup
{
public:
			uiTabStack( uiParent*, const char* nm,
				    bool manage=true );

			//! returns id of  new tab
    void		addTab( uiGroup*, const char* txt =0 );
//    void		addTab( uiGroup&, const ioPixmap&, const char* txt=0 );
    void		insertTab( uiGroup*, const char*, int index = -1 );
//    void		insertTab( uiGroup&, const ioPixmap&, const char* txt=0,
//				   int index = -1 );
    void		removeTab( int id );

    void		setTabEnabled( uiGroup*, bool );
    bool		isTabEnabled( uiGroup* ) const;

/*
    void		removeTabToolTip( uiGroup* );
    void		setTabToolTip( uiGroup*, const char* tip );
    const char*		tabToolTip( uiGroup* ) const;
*/

    void		setCurrentPage( int id );
    void		setCurrentPage( uiGroup* );

    uiGroup*		currentPage() const;
    uiGroup* 		page( int id ) const;
    int			idOf( uiGroup* ) const;
    int			currentPageId() const;

    int			size() const;
/*
    const char*		tabText( int idx ) const;
    const char*		tabText( uiGroup* ) const;
    void		setTabText( uiGroup*, const char* txt );
*/
    uiGroup*		tabGroup() { return tabgrp_; }

protected:

    uiTabBar*		tabbar_;
    uiGroup*		tabgrp_;

    void		tabSel( CallBacker* cb=0 );

    mutable BufferString rettxt;
};


#endif
