/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.1 2007-07-09 10:12:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uistratreftree.h"
#include "uistratunitdesctab.h"
#include "uitable.h"


uiStratTreeWin::uiStratTreeWin( uiParent* p, const Strat::RefTree* rt )
    : uiMainWin(p,"Strat RefTree viewer group", 0, true, true)
{
    createMenus();
    uitree_ = new uiStratRefTree( this, rt );
    const_cast<uiStratRefTree*>(uitree_)->listView()->selectionChanged.notify( 
	    				mCB( this,uiStratTreeWin,unitSelCB ) );
    uistrattab_ = new uiStratUnitDescTab( this, 0 );
    uistrattab_->table()->attach( alignedBelow, (uiObject*)uitree_->listView());
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete uitree_;
    delete uistrattab_;
}

    
void uiStratTreeWin::createMenus()
{
    uiMenuBar* menubar =  menuBar();
    uiPopupMenu* viewmnu = new uiPopupMenu( this, "&View" );
    expandmnuitem_ = new uiMenuItem( "Expand all",
					mCB(this, uiStratTreeWin, setExpCB ) );
    viewmnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setCheckable( true );
    expandmnuitem_->setChecked( true );
    menubar->insertItem( viewmnu );
	    
}


void uiStratTreeWin::setExpCB(CallBacker*)
{
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    uiListViewItem* item = uitree_->listView()->selectedItem();
    CompoundKey kc;
    for ( int idx=item->depth()-1; idx>=0; idx-- )
	kc += item->parent()->text();
    kc += item->text();
    uistrattab_->setUnitRef( uitree_->findUnit( kc.buf() ) );
}
