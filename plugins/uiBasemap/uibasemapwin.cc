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
#include "uibasemaptbmgr.h"
#include "uidockwin.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uimenu.h"
#include "uistrings.h"
#include "uisurvmap.h"
#include "uitoolbutton.h"
#include "uitreeview.h"
#include "uiworld2ui.h"

#include "mouseevent.h"
#include "survinfo.h"


uiBasemapWin::uiBasemapWin( uiParent* p )
    : uiMainWin(p,Setup("Basemap").withmenubar(false).nrstatusflds(3)
				  .deleteonclose(false))
    , topitem_(0)
    , mousecursorexchange_(0)
{
    initView();
    initTree();
    tbmgr_ = new uiBaseMapTBMgr( *this, *basemapview_ );

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


void uiBasemapWin::initView()
{
    basemapview_ = new uiSurveyMap( this, true, true, true );
    basemapview_->setPrefHeight( 250 );
    basemapview_->setPrefWidth( 250 );
    basemapview_->setSurveyInfo( &SI() );
    basemapview_->view().setMouseTracking( true );
    basemapview_->view().enableScrollZoom();
    basemapview_->view().setSceneBorder( 20 );

    basemapview_->view().getMouseEventHandler().movement.notify(
	mCB(this,uiBasemapWin,mouseMoveCB) );
    basemapview_->view().scene().setMouseEventActive( true );
}


void uiBasemapWin::initTree()
{
    treedw_ = new uiDockWin( this, "Basemap Tree" );
    addDockWindow( *treedw_, uiMainWin::Left );

    tree_ = new uiTreeView( treedw_ );
    tree_->setColumnText( 0, "Elements" );
    tree_->setSelectionMode( uiTreeView::Extended );
    tree_->setRootDecorated( false );
    tree_->showHeader( false );
    topitem_ = new uiBasemapTreeTop( tree_ );
}


void uiBasemapWin::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.remove(
		mCB(this,uiBasemapWin,mouseCursorExchangeCB) );

    mousecursorexchange_ = mce;

    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.notify(
		mCB(this,uiBasemapWin,mouseCursorExchangeCB) );
}


void uiBasemapWin::mouseCursorExchangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,caller,cb);
    if ( caller==this )
        return;

    BMM().updateMouseCursor( info.surveypos_ );
}


void uiBasemapWin::mouseMoveCB( CallBacker* )
{
    const MouseEvent& ev = basemapview_->view().getMouseEventHandler().event();
    const Coord crd( basemapview_->transform().toWorldX( ev.x() ),
		     basemapview_->transform().toWorldY( ev.y() ) );
    if ( !crd.isDefined() )
    { toStatusBar( "", 0 ); return; }

    const BinID bid = SI().transform( crd );
    BufferString istr;
    const BufferString xstr = toString( crd.x, 0 );
    const BufferString ystr = toString( crd.y, 0 );
    istr.add( bid.inl() ).add( "/" ).add( bid.crl() ).add( " (" )
	.add( xstr.buf() ).add( " , " ).add( ystr.buf() ).add( ")" );
    toStatusBar( istr, 0 );

    const FixedString itmnm = basemapview_->nameOfItemAt( ev.pos() );
    toStatusBar( itmnm.buf(), 2 );

    MouseCursorExchange::Info info( Coord3(crd,0) );
    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.trigger( info, this );
}


BaseMap* uiBasemapWin::getBasemap()
{ return basemapview_; }


bool uiBasemapWin::closeOK()
{
    saveSettings();
    return true;
}
