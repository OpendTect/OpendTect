/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicoltabmarker.cc,v 1.12 2011/03/31 09:24:38 cvsnanne Exp $";

#include "uicoltabmarker.h"

#include "uicolor.h"
#include "uigraphicsscene.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitable.h"
#include "uiworld2ui.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabsequence.h"
#include "draw.h"
#include "mouseevent.h"
#include "rowcol.h"
#include <math.h>


static const int cColorCol = 1;
#define mEps 0.00001

uiColTabMarkerDlg::uiColTabMarkerDlg( uiParent* p, ColTab::Sequence& ctab )
    : uiDialog(p,uiDialog::Setup("Manage Marker","Add, remove, change Markers",
				 "50.1.1"))
    , ctab_(ctab)
    , markersChanged(this)
{
    table_ = new uiTable( this, uiTable::Setup(ctab_.size(),2).rowgrow(true)
	    					.rowdesc("Marker")
						.defrowlbl(true)
	   					.manualresize(true),
						"Marker table");
    BufferStringSet columnlabels;
    columnlabels.add("Position").add("Color");
    table_->setColumnLabels( columnlabels );
    table_->setColumnReadOnly( cColorCol, true );
    fillTable();

    table_->leftClicked.notify( mCB(this,uiColTabMarkerDlg,mouseClick) );
    table_->rowInserted.notify( mCB(this,uiColTabMarkerDlg,markerInserted) );
    table_->rowDeleted.notify( mCB(this,uiColTabMarkerDlg,markerDeleted) );
    table_->valueChanged.notify( mCB(this,uiColTabMarkerDlg,markerPosChgd) );
}


void uiColTabMarkerDlg::fillTable()
{
    for ( int idx=0; idx<table_->nrCols(); idx++ )
    {
	for ( int idy=0; idy<ctab_.size(); idy++ )
	{
	    RowCol rc;
	    rc.row = idy;
	    rc.col = idx;
	    const float position = ctab_.position( idy );
	    if ( rc.col == 0 )
		table_->setValue( rc, position );
	    if ( rc.col == 1 )
	    {
		Color color( ctab_.color(position) );
		table_->setColor( rc, color );
	    }
	}
    }
}


void uiColTabMarkerDlg::mouseClick( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rc = table_->notifiedCell();
    if ( rc.col != cColorCol ) return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
    {
	ColTab::Sequence orgctab = ctab_;
	table_->setColor( rc, newcol );
	table_->setCurrentCell( RowCol(rc.row,0) );
	orgctab.changeColor( rc.row, newcol.r(), newcol.g(), newcol.b() );
	ctab_ = orgctab;
    }

    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerInserted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcvalue = table_->newCell();
    if ( rcvalue.row-1 < 0 || rcvalue.row >= ctab_.size() )
    {
	table_->removeRow( rcvalue );
	uiMSG().error( "Cannot insert markers at extreme positions" );
	return;
    }

    RowCol rccolor = ( rcvalue.row, 1 );
    const float newpos = ctab_.position(rcvalue.row-1) +
			 ( ctab_.position(rcvalue.row) - 
			   ctab_.position(rcvalue.row-1) ) / 2;
    Color col( ctab_.color(newpos) );
    table_->setColor( rccolor, col );
    table_->setCurrentCell( RowCol(rcvalue.row,0) );
    ctab_.setColor( newpos, col.r(), col.g(), col.b() );
    fillTable();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerDeleted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    if ( rc.row == 0 || rc.row == ctab_.size()-1 )
    {
	table_->insertRows( rc, 1 );
	const float pos = ctab_.position(rc.row);
	table_->setValue( RowCol(rc.row,0), pos );
	table_->setColor( RowCol(rc.row,1), ctab_.color(pos) );
	table_->setCurrentCell( RowCol(rc.row,0) );
	uiMSG().error( "Cannot remove markers at extreme positions" );
	return;
    }

    fillTable();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerPosChgd( CallBacker* )
{
    RowCol rc = table_->currentCell();
    const float newpos = table_->getfValue( rc );
    if ( ctab_.position(rc.row-1)>newpos || ctab_.position(rc.row+1)<newpos )
    {
	uiMSG().error( "Please enter position between 0 & 1 for Markers " );
	table_->setValue( rc, ctab_.position(rc.row) );
	return;
    }
    ctab_.changePos( rc.row, newpos );
    markersChanged.trigger();
}


void uiColTabMarkerDlg::rebuildColTab()
{
    ColTab::Sequence orgctab = ctab_;
    ctab_.removeAllColors();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	RowCol newtable( idx, 0 );
	RowCol coltable( idx, 1 );
	const float newpos = table_->getfValue( newtable );
	Color col( orgctab.color(newpos) );
	table_->setColor( coltable, col );
	ctab_.setColor( newpos, col.r(), col.g(), col.b() );
    }
}


bool uiColTabMarkerDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	RowCol rc( idx, 0 );
	const float position = table_->getfValue( rc );
	Color col( table_->getColor( RowCol(idx,1) ) );
	ctab_.changeColor( idx, col.r(), col.g(), col.b() );
    }

    return true;
}


// ***** uiColTabMarkerCanvas ****
uiColTabMarkerCanvas::uiColTabMarkerCanvas( uiParent* p, ColTab::Sequence& ctab)
    : uiGraphicsView(p,"Marker Canvas")
    , parent_(p)
    , ctab_(ctab)
    , markerChanged(this)
    , markerlineitmgrp_(0)
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
    scene().setSceneRect( 0, 0, width(), height() );
    w2ui_->set( uiRect(0,0,width()-5,height()-5), uiWorldRect(0,255,1,0) );

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
	uiWorldPoint wpt = uiWorldPoint( val, 0 );
	uiPoint pt( w2ui_->transform(wpt) );
	uiLineItem* lineitem = new uiLineItem();
	lineitem->setLine( pt.x, 0, pt.x, 15 );
	lineitem->setPenStyle( LineStyle(LineStyle::Solid,3) );
	markerlineitmgrp_->add( lineitem );
    }
}


void uiColTabMarkerCanvas::mouseClk( CallBacker* cb )
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
	const float ref = wpt.x;
	const float diffinpix = fabs(val-ref) / fabs(fac);
	if ( diffinpix < mindiff )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( OD::RightButton != ev.buttonState() )
	return;

    uiPopupMenu mnu( parent_, "Action" );
    if ( selidx_>=0 )
    {
	if ( selidx_ != 0 && selidx_ != ctab_.size()-1 )
	mnu.insertItem( new uiMenuItem("Remove color"), 0 );
	mnu.insertItem( new uiMenuItem("Change color ..."), 1 );
    }
    
    mnu.insertItem( new uiMenuItem("Edit Markers ..."), 2 );

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
    const Color col = ctab_.color( pos );
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
    Color col = ctab_.color( ctab_.position(markeridx) );
    if ( !selectColor(col,parent_,"Color selection",false) )
	return false;

    ctab_.changeColor( markeridx, col.r(), col.g(), col.b() );
    markerChanged.trigger();
    return true;
}


void uiColTabMarkerCanvas::mouse2Clk( CallBacker* cb )
{
    if ( meh_.isHandled() )
    return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    addMarker( wpt.x, true );
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


void uiColTabMarkerCanvas::mouseMove( CallBacker* cb )
{
    NotifyStopper notifstop( meh_.buttonPressed );
    if ( meh_.isHandled() )
	return;
    if ( selidx_<=0 || selidx_==ctab_.size()-1 ) return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    float changepos = wpt.x;

    const int sz = ctab_.size();
    if ( selidx_<0 || selidx_>=sz ) return;

    if ( changepos > 1 ) changepos = 1;
    if ( changepos < 0 ) changepos = 0;

    float position = mUdf(float); 
    if ( (selidx_ > 0 && ctab_.position(selidx_-1)>=changepos) )
	position = ctab_.position(selidx_-1) + 1.01*mEps;
    else if ( (selidx_ < sz-1 && ctab_.position(selidx_+1)<=changepos) )
	position = ctab_.position( selidx_+1 ) - 1.01*mEps;
    else
	position = changepos;

    ctab_.changePos( selidx_, position );
    reDrawNeeded.trigger();
    markerChanged.trigger();
    meh_.setHandled( true );
}
