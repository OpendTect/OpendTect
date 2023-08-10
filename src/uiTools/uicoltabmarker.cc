/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabmarker.h"

#include "uicolor.h"
#include "uigraphicsscene.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitable.h"
#include "uiworld2ui.h"

#include "color.h"
#include "coltabsequence.h"
#include "draw.h"
#include "mouseevent.h"
#include "rowcol.h"
#include "od_helpids.h"


static const int sPosCol = 0;
static const int sColorCol = 1;

#define mEps 0.00001

uiColTabMarkerDlg::uiColTabMarkerDlg( uiParent* p, ColTab::Sequence& ctab )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage(tr("Color Anchors")),
				tr("Add, Remove, Change Anchors"),
				 mODHelpKey(mColTabMarkerDlgHelpID) ))
    , markersChanged(this)
    , ctab_(ctab)
{
    table_ = new uiTable( this, uiTable::Setup(ctab_.size(),2)
						.rowgrow(true)
						.rowdesc(tr("Anchor"))
						.defrowlbl(true)
						.manualresize(true)
						.removeselallowed(false),
			  "Marker table");
    uiStringSet columnlabels;
    columnlabels.add( uiStrings::sPosition() )
		.add( uiStrings::sColor() );
    table_->setColumnLabels( columnlabels );
    table_->setColumnReadOnly( sColorCol, true );
    table_->setSelectionMode( uiTable::SingleRow );
    fillTable();

    mAttachCB( table_->leftClicked, uiColTabMarkerDlg::mouseClick );
    mAttachCB( table_->rowInserted, uiColTabMarkerDlg::markerInserted );
    mAttachCB( table_->rowDeleted, uiColTabMarkerDlg::markerDeleted );
    mAttachCB( table_->valueChanged, uiColTabMarkerDlg::markerPosChgd );
}


uiColTabMarkerDlg::~uiColTabMarkerDlg()
{
    detachAllNotifiers();
}


void uiColTabMarkerDlg::fillTable()
{
    for ( int cidx=0; cidx<ctab_.size(); cidx++ )
    {
	const float position = ctab_.position( cidx );
	table_->setValue( RowCol(cidx,sPosCol), position );
	table_->setColor( RowCol(cidx,sColorCol), ctab_.color(position) );
    }
}


void uiColTabMarkerDlg::mouseClick( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    if ( rc.col() != sColorCol )
	return;

    OD::Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,tr("Anchor color")) )
    {
	ColTab::Sequence orgctab = ctab_;
	table_->setColor( rc, newcol );
	table_->setCurrentCell( RowCol(rc.row(),sPosCol) );
	orgctab.changeColor( rc.row(), newcol );
	ctab_ = orgctab;
    }

    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerInserted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcpos = table_->newCell();
    if ( rcpos.row()-1<0 || rcpos.row()>=ctab_.size() )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert achors at extreme positions") );
	return;
    }

    const RowCol rccolor( rcpos.row(), sColorCol );
    const float newpos = ctab_.position(rcpos.row()-1) +
			 ( ctab_.position(rcpos.row()) -
			   ctab_.position(rcpos.row()-1) ) / 2;
    const OD::Color col( ctab_.color(newpos) );
    table_->setColor( rccolor, col );
    table_->setCurrentCell( RowCol(rcpos.row(),sPosCol) );
    ctab_.setColor( newpos, col );
    fillTable();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerDeleted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    if ( rc.row()==0 || rc.row()==ctab_.size()-1 )
    {
	table_->insertRows( rc, 1 );
	const float pos = ctab_.position( rc.row() );
	table_->setValue( RowCol(rc.row(),sPosCol), pos );
	table_->setColor( RowCol(rc.row(),sColorCol), ctab_.color(pos) );
	table_->setCurrentCell( RowCol(rc.row(),sPosCol) );
	uiMSG().error( tr("Cannot remove markers at extreme positions") );
	return;
    }

    ctab_.removeColor( rc.row() );
    fillTable();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerPosChgd( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    const float newpos = table_->getFValue( rc );
    if (ctab_.position(rc.row()-1)>newpos || ctab_.position(rc.row()+1)<newpos)
    {
	uiMSG().error( tr("Please enter position between 0 and 1") );
	table_->setValue( rc, ctab_.position(rc.row()) );
	return;
    }

    ctab_.changePos( rc.row(), newpos );
    markersChanged.trigger();
}


void uiColTabMarkerDlg::rebuildColTab()
{
    ColTab::Sequence orgctab = ctab_;
    ctab_.removeAllColors();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const RowCol posrc( idx, sPosCol );
	const RowCol colrc( idx, sColorCol );
	const float newpos = table_->getFValue( posrc );
	const OD::Color col( orgctab.color(newpos) );
	table_->setColor( colrc, col );
	ctab_.setColor( newpos, col );
    }
}


bool uiColTabMarkerDlg::acceptOK( CallBacker* )
{
    NotifyStopper ns( ctab_.colorChanged );
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const RowCol colrc( idx, sColorCol );
	const OD::Color col( table_->getColor(colrc) );
	ctab_.changeColor( idx, col );
    }

    ctab_.colorChanged.trigger();
    return true;
}


// ***** uiColTabMarkerCanvas ****
uiColTabMarkerCanvas::uiColTabMarkerCanvas( uiParent* p, ColTab::Sequence& ctab)
    : uiGraphicsView(p,"Marker Canvas")
    , markerChanged(this)
    , parent_(p)
    , markerlineitmgrp_(0)
    , ctab_(ctab)
    , meh_(scene().getMouseEventHandler())
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    selidx_ = mUdf(int);
    w2ui_ = new uiWorld2Ui( uiWorldRect(0,255,1,0), uiSize(1,1) );

    reDrawNeeded.notify( mCB(this,uiColTabMarkerCanvas,drawMarkers));
    reSize.notify( mCB(this,uiColTabMarkerCanvas,drawMarkers));

    meh_.buttonPressed.notify(mCB(this,uiColTabMarkerCanvas,mouseClk) );
    meh_.movement.notify( mCB(this,uiColTabMarkerCanvas,mouseMove) );
    meh_.doubleClick.notify( mCB(this,uiColTabMarkerCanvas,mouse2Clk) );
    meh_.buttonReleased.notify(mCB(this,uiColTabMarkerCanvas,mouseRelease) );
}


uiColTabMarkerCanvas::~uiColTabMarkerCanvas()
{
    delete w2ui_;
}


void uiColTabMarkerCanvas::drawMarkers( CallBacker* )
{
    const int w = viewWidth();
    const int h = viewHeight();
    scene().setSceneRect( 0, 0, sCast(float,w), sCast(float,h) );
    w2ui_->set( uiRect(0,0,w-5,h-5), uiWorldRect(0,255,1,0) );

    if ( !markerlineitmgrp_ )
    {
	markerlineitmgrp_ = new uiGraphicsItemGroup();
	scene().addItem( markerlineitmgrp_ );
    }
    else
	markerlineitmgrp_->removeAll( true );

    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
	const float val = ctab_.position(idx);
	uiWorldPoint wpt( val, 0 );
	const uiPoint pt( w2ui_->transform(wpt) );
	auto* lineitem = new uiLineItem();
	lineitem->setLine( pt.x, 0, pt.x, 15 );
	lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,3) );
	markerlineitmgrp_->add( lineitem );
    }
}


void uiColTabMarkerCanvas::mouseClk( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );

    selidx_ = -1;
    float mindiff = 5;
    uiWorldPoint wpp = w2ui_->worldPerPixel();
    float fac = (float)wpp.x;
    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
	const float val = ctab_.position( idx );
	const float ref = (float) ( wpt.x );
	const float diffinpix = Math::Abs(val-ref) / Math::Abs(fac);
	if ( diffinpix < mindiff )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( OD::RightButton != ev.buttonState() )
	return;

    uiMenu mnu( parent_, uiStrings::sAction() );
    if ( selidx_>=0 )
    {
	if ( selidx_ != 0 && selidx_ != ctab_.size()-1 )
	mnu.insertAction( new uiAction(tr("Remove color")), 0 );
	mnu.insertAction( new uiAction(m3Dots(tr("Change color"))), 1 );
    }

    mnu.insertAction( new uiAction(m3Dots(tr("Edit color anchors"))), 2 );

    const int res = mnu.exec();
    if ( res==0 )
	removeMarker( selidx_ );
    else if ( res==1 )
	changeColor( selidx_ );
    else if ( res==2 )
    {
	ColTab::Sequence coltab = ctab_;
	uiColTabMarkerDlg dlg( parent_, ctab_ );
	dlg.markersChanged.notify( mCB(this,uiColTabMarkerCanvas,markerChgd) );
	if ( !dlg.go() )
	{
	    ctab_ = coltab;
	    markerChgd(0);
	}
    }

    selidx_ = -1;
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::markerChgd( CallBacker* )
{
    markerChanged.trigger();
    reDrawNeeded.trigger();
}


void uiColTabMarkerCanvas::addMarker( float pos, bool withcolsel )
{
    ColTab::Sequence coltab = ctab_;
    const OD::Color col = ctab_.color( pos );
    const int markeridx = ctab_.setColor( pos, col.r(), col.g(), col.b() );

    if ( withcolsel )
    {
	if ( !changeColor( markeridx ) )
	    ctab_ = coltab;
    }

    reDrawNeeded.trigger();
}


void uiColTabMarkerCanvas::removeMarker( int markeridx )
{
    ctab_.removeColor( markeridx );
    markerChanged.trigger();
    reDrawNeeded.trigger();
}


bool uiColTabMarkerCanvas::changeColor( int markeridx )
{
    OD::Color col = ctab_.color( ctab_.position(markeridx) );
    if ( !selectColor(col,parent_,tr("Color selection"),false) )
	return false;

    ctab_.changeColor( markeridx, col.r(), col.g(), col.b() );
    markerChanged.trigger();
    return true;
}


void uiColTabMarkerCanvas::mouse2Clk( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    const MouseEvent& ev = meh_.event();
    const uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    addMarker( sCast(float,wpt.x), true );
    selidx_ = -1;
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::mouseRelease( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    selidx_ = -1;
    reDrawNeeded.trigger();
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::mouseMove( CallBacker* )
{
    NotifyStopper notifstop( meh_.buttonPressed );
    if ( meh_.isHandled() )
	return;

    if ( selidx_<=0 || selidx_==ctab_.size()-1 )
	return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    float changepos = (float) ( wpt.x );

    const int sz = ctab_.size();
    if ( selidx_<0 || selidx_>=sz )
	return;

    if ( changepos > 1 ) changepos = 1;
    if ( changepos < 0 ) changepos = 0;

    float position = mUdf(float);
    if ( (selidx_ > 0 && ctab_.position(selidx_-1)>=changepos) )
	position = (float) ( ctab_.position(selidx_-1) + 1.01*mEps );
    else if ( (selidx_ < sz-1 && ctab_.position(selidx_+1)<=changepos) )
	position = (float) ( ctab_.position( selidx_+1 ) - 1.01*mEps );
    else
	position = changepos;

    ctab_.changePos( selidx_, position );
    reDrawNeeded.trigger();
    markerChanged.trigger();
    meh_.setHandled( true );
}
