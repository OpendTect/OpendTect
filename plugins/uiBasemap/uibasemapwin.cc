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
#include "uibasemapscalebar.h"
#include "uibasemaptbmgr.h"
#include "uidockwin.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uimenu.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uisurvmap.h"
#include "uitoolbutton.h"
#include "uitreeview.h"
#include "uiworld2ui.h"

#include "mouseevent.h"
#include "survinfo.h"


uiBasemapView::uiBasemapView( uiParent* p )
    : uiBaseMap(p)
{
    addStdItems();
    init();

    view().setMouseTracking( true );
    view().enableScrollZoom();
    view().setSceneBorder( 10 );
    view().scene().setMouseEventActive( true );
}


uiBasemapView::~uiBasemapView()
{
    view().scene().removeItem( northarrow_ );
    delete crosshair_;
}


void uiBasemapView::addStdItems()
{
    const SurveyInfo& si = SI();

    survbox_ = new uiSurveyBoxObject( 0 );
    survbox_->setSurveyInfo( &si );
    survbox_->setLineStyle( LineStyle(LineStyle::Dot,1,Color::Red()) );
    survbox_->showLabels( false );
    addObject( survbox_ );

    const uiPixmap pm( "northarrow" );
    northarrow_ = view().scene().addItem( new uiPixmapItem(pm) );
    northarrow_->setScale( 0.5, 0.5 );
    northarrow_->setMovable( true );
    northarrow_->setItemIgnoresTransformations( true );
    northarrow_->setPos( 10, 10 );

    scalebar_ = new uiMapScaleObject( 0 );
    scalebar_->setSurveyInfo( &si );
    addStaticObject( scalebar_ );

    crosshair_ = new uiCrossHairItem( view() );
}


void uiBasemapView::init()
{
    setPrefHeight( 250 );
    setPrefWidth( 250 );

    const Coord mincoord = SI().minCoord( false );
    const Coord maxcoord = SI().maxCoord( false );
    const double diffx = maxcoord.x - mincoord.x;
    const double diffy = maxcoord.y - mincoord.y;
    const uiWorldRect wr( mincoord.x-diffx/4, maxcoord.y+diffy/4,
			  maxcoord.x+diffx/4, mincoord.y-diffy/4 );
    setView( wr );
}


uiCrossHairItem* uiBasemapView::getCrossHair()	{ return crosshair_; }
void uiBasemapView::showCrossHair( bool yn )	{ crosshair_->show( yn ); }
bool uiBasemapView::crossHairShown() const	{ return crosshair_->isShown();}

uiPixmapItem* uiBasemapView::getNorthArrow()	{ return northarrow_; }
void uiBasemapView::showNorthArrow( bool yn )	{ northarrow_->setVisible(yn); }
bool uiBasemapView::northArrowShown() const { return northarrow_->isVisible(); }

uiMapScaleObject* uiBasemapView::getScaleBar()	{ return scalebar_; }
void uiBasemapView::showScaleBar( bool yn )	{ scalebar_->show( yn ); }
bool uiBasemapView::scaleBarShown() const	{ return scalebar_->isShown(); }

uiSurveyBoxObject* uiBasemapView::getSurveyBox() { return survbox_; }
void uiBasemapView::showSurveyBox( bool yn )	{ survbox_->show( yn ); }
bool uiBasemapView::surveyBoxShown() const	{ return survbox_->isShown(); }


void uiBasemapView::reDraw( bool deep )
{
    scalebar_->setPixelPos( mCast(int,view().scene().width()),
			    mCast(int,view().scene().height()) );
    uiBaseMap::reDraw( deep );
}


// uiBasemapWin
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
{
    basemapview_->view().getMouseEventHandler().movement.remove(
	mCB(this,uiBasemapWin,mouseMoveCB) );
    basemapview_->view().getMouseEventHandler().buttonPressed.remove(
	mCB(this,uiBasemapWin,mouseClickCB) );
}


void uiBasemapWin::initWin( CallBacker* )
{
    readSettings();
}


void uiBasemapWin::initView()
{
    basemapview_ = new uiBasemapView( this );
    basemapview_->view().getMouseEventHandler().movement.notify(
	mCB(this,uiBasemapWin,mouseMoveCB) );
    basemapview_->view().getMouseEventHandler().buttonPressed.notify(
	mCB(this,uiBasemapWin,mouseClickCB) );
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


void uiBasemapWin::mouseClickCB(CallBacker *)
{
    if ( !SI().has3D() ) return;

    const MouseEvent& ev = basemapview_->view().getMouseEventHandler().event();
    if ( !ev.rightButton() ) return;

    const Coord crd( basemapview_->getWorld2Ui().toWorldX(ev.x()),
		     basemapview_->getWorld2Ui().toWorldY(ev.y()) );
    if ( !crd.isDefined() ) return;

    const BinID bid = SI().transform( crd );
    if ( !SI().sampling(false).hsamp_.includes(bid) )
	return;

    uiMenu* mnu = new uiMenu( "Menu" );
    mnu->insertAction( new uiAction("Add In-line"), 0 );
    mnu->insertAction( new uiAction("Add Cross-line"), 1 );
    mnu->insertAction( new uiAction("Add In-line and Cross-line"), 2 );
    const int mnuid = mnu->exec();
    const bool addinl = mnuid==0 || mnuid==2;
    const bool addcrl = mnuid==1 || mnuid==2;
    if ( addinl )
	ODMainWin()->sceneMgr().addInlCrlItem( OD::InlineSlice, bid.inl() );
    if ( addcrl )
	ODMainWin()->sceneMgr().addInlCrlItem( OD::CrosslineSlice, bid.crl() );
}


void uiBasemapWin::mouseMoveCB( CallBacker* )
{
    const MouseEvent& ev = basemapview_->view().getMouseEventHandler().event();
    const Coord crd( basemapview_->getWorld2Ui().toWorldX(ev.x()),
		     basemapview_->getWorld2Ui().toWorldY(ev.y()) );
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
