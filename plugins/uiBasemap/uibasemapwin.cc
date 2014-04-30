/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapwin.h"

#include "uibasemapitem.h"
#include "uidockwin.h"
#include "uisurvmap.h"
#include "uitoolbar.h"
#include "uitreeview.h"

#include "survinfo.h"


uiBasemapWin::uiBasemapWin( uiParent* p )
    : uiMainWin(p,Setup("Basemap").withmenubar(false).nrstatusflds(3)
				  .deleteonclose(false))
    , topitem_(0)
{
    basemapview_ = new uiSurveyMap( this );
    basemapview_->setPrefHeight( 250 );
    basemapview_->setPrefWidth( 250 );
    basemapview_->setSurveyInfo( &SI() );

    treedw_ = new uiDockWin( this, "Basemap Tree" );
    addDockWindow( *treedw_, uiMainWin::Left );

    tree_ = new uiTreeView( treedw_ );
    initTree();
    initToolBar();

    postFinalise().notify( mCB(this,uiBasemapWin,initWin) );
}


uiBasemapWin::~uiBasemapWin()
{}


void uiBasemapWin::initWin( CallBacker* )
{
    readSettings();
}


void uiBasemapWin::initTree()
{
    tree_->setColumnText( 0, "Elements" );
    topitem_ = new uiBasemapTreeTop( tree_ );
}


void uiBasemapWin::initToolBar()
{
    toolbar_ = new uiToolBar( this, "Basemap Items" );

    CallBack cb = mCB(this,uiBasemapWin,iconClickCB);
    const BufferStringSet& nms = uiBasemapItem::factory().getNames();
    const TypeSet<uiString>& usrnms = uiBasemapItem::factory().getUserNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBasemapItem* itm =
		uiBasemapItem::factory().create( nms.get(idx) );
	itm->setBasemap( *basemapview_ );
	itm->setTreeTop( *topitem_ );

	uiString str( "Add " ); str.append( usrnms[idx] );
	uiAction* action = new uiAction( str, cb, itm->iconName() );
	ids_ += toolbar_->insertAction( action );
	items_ += itm;
    }
}


void uiBasemapWin::iconClickCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb)
    if ( !action ) return;

    const int id = action->getID();
    const int itmidx = ids_.indexOf( id );
    if ( !items_.validIdx(itmidx) ) return;

    items_[itmidx]->add();
}


bool uiBasemapWin::closeOK()
{
    saveSettings();
    return true;
}
