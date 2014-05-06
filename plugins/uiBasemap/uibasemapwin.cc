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
#include "uigraphicsview.h"
#include "uisurvmap.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
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
    uiGraphicsView::ScrollBarPolicy sbpol = uiGraphicsView::ScrollBarAsNeeded;
    basemapview_->view().setScrollBarPolicy( true, sbpol );
    basemapview_->view().setScrollBarPolicy( false, sbpol );

    treedw_ = new uiDockWin( this, "Basemap Tree" );
    addDockWindow( *treedw_, uiMainWin::Left );

    tree_ = new uiTreeView( treedw_ );
    initTree();
    initToolBar();

    BMM().setBasemap( *basemapview_ );
    BMM().setTreeTop( *topitem_ );
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
    toolbar_->addObject(
	basemapview_->view().getSaveImageButton(toolbar_) );

    CallBack cb = mCB(this,uiBasemapWin,iconClickCB);
    const BufferStringSet& nms = uiBasemapItem::factory().getNames();
    const TypeSet<uiString>& usrnms = uiBasemapItem::factory().getUserNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBasemapItem* itm =
		uiBasemapItem::factory().create( nms.get(idx) );

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

    BMM().add( items_[itmidx]->factoryKeyword() );
}


bool uiBasemapWin::closeOK()
{
    saveSettings();
    return true;
}
