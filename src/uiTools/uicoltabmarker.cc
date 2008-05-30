/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabmarker.cc,v 1.1 2008-05-30 04:10:33 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicoltabmarker.h"

#include "uicolor.h"
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


static const int sColorCol = 1;
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
    table_->setColumnReadOnly( sColorCol, true );
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
    if ( rc.col != sColorCol ) return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
    {
	ColTab::Sequence orgctab = ctab_;
	table_->setColor( rc, newcol );
	table_->setCurrentCell( RowCol(rc.row,0) );
	orgctab.changeColor( rc.row, newcol.r(), newcol.g(), newcol.b() );
	ctab_ = orgctab;
    }
    rebuildColTab();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerInserted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcvalue = table_->newCell();
    if ( rcvalue.row-1 < 0 || rcvalue.row >= ctab_.size() )
    {
	table_->removeRow( rcvalue );
	uiMSG().error( "Cannot insert markers at extreme positions " );
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

    rebuildColTab();
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
	const float position = table_->getValue( rc );
	Color col( table_->getColor( RowCol(idx,1) ) );
	ctab_.changeColor( idx, col.r(), col.g(), col.b() );
    }
    return true;
}


// ***** uiColTabMarkerCanvas ****
uiColTabMarkerCanvas::uiColTabMarkerCanvas( uiParent* p, ColTab::Sequence& ctab,
					    const int prefwidth )
    : uiCanvas(p,Color::White,"Marker Canvas")
    , parent_(p)
    , ctab_(ctab)
    , markerChanged(this)
{
    selidx_ = mUdf(int);
    w2ui_ = new uiWorld2Ui( uiWorldRect(0,255,1,0),
	    		    uiSize(prefwidth,prefwidth/15) );
    setStretch( 0, 0 );
    setPrefWidth( prefwidth );
    setPrefHeight( prefwidth/15 );

    postDraw.notify( mCB(this,uiColTabMarkerCanvas,drawMarkers));

    MouseEventHandler& meh = getMouseEventHandler();
    meh.buttonPressed.notify(mCB(this,uiColTabMarkerCanvas,mouseClk) );
    meh.movement.notify( mCB(this,uiColTabMarkerCanvas,mouseMove) );
    meh.doubleClick.notify( mCB(this,uiColTabMarkerCanvas,mouse2Clk) );
    meh.buttonReleased.notify(mCB(this,uiColTabMarkerCanvas,mouseRelease) );
}


uiColTabMarkerCanvas::~uiColTabMarkerCanvas()
{
    delete w2ui_;
}


void uiColTabMarkerCanvas::drawMarkers( CallBacker* )
{
    ioDrawTool& dt = drawTool();
    dt.setPenColor( Color::Black );
    dt.setBackgroundColor( Color::White );
    dt.clear();
    dt.setFillColor( Color::Black );
    LineStyle ls( LineStyle::Solid );
    ls.width_ = 3;
    dt.setLineStyle( ls );

    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
	float val = ctab_.position(idx);
	uiWorldPoint wpt = uiWorldPoint(val,0);
	uiPoint pt( w2ui_->transform(wpt) );
	dt.drawLine( pt.x, 0, pt.x, 15 );
    }

}


void uiColTabMarkerCanvas::mouseClk( CallBacker* cb )
{
    if ( getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev = getMouseEventHandler().event();
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
	if ( isSegmentized() )
	{
	    uiMSG().error( "You cannot edit markers of segementized table");
	    return;
	}
	uiColTabMarkerDlg dlg( parent_, ctab_ );
	dlg.markersChanged.notify( mCB(this,uiColTabMarkerCanvas,markerChgd) );
	if ( !dlg.go() )
	    ctab_ = coltab;
	markerChanged.trigger();
	update();
    }

    update();
    selidx_ = -1;
    getMouseEventHandler().setHandled( true );
}


void uiColTabMarkerCanvas::markerChgd( CallBacker* )
{
    markerChanged.trigger();
    update();
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
}


void uiColTabMarkerCanvas::removeMarker( int markeridx )
{
    ctab_.removeColor( markeridx );
    markerChanged.trigger();
    update();
}


bool uiColTabMarkerCanvas::changeColor( int markeridx )
{
    Color col = ctab_.color( ctab_.position(markeridx) );
    if ( selectColor(col,parent_,"Color selection",false) )
    {
	ctab_.changeColor( markeridx, col.r(), col.g(), col.b() );
	markerChanged.trigger();
	update();
	return true;
    }
    return false;
}


void uiColTabMarkerCanvas::mouse2Clk( CallBacker* cb )
{
    if ( getMouseEventHandler().isHandled() )
    return;

    const MouseEvent& ev = getMouseEventHandler().event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    addMarker( wpt.x, true );
    update();
    selidx_ = -1;
    getMouseEventHandler().setHandled( true );
}


void uiColTabMarkerCanvas::mouseRelease( CallBacker* )
{
    if ( getMouseEventHandler().isHandled() )
	return;

    selidx_ = -1;
    update();
    getMouseEventHandler().setHandled( true );
}


void uiColTabMarkerCanvas::mouseMove( CallBacker* cb )
{
    NotifyStopper notifstop( getMouseEventHandler().buttonPressed );
    if ( getMouseEventHandler().isHandled() )
	return;
    if ( selidx_<=0 || selidx_==ctab_.size()-1 ) return;

    const MouseEvent& ev = getMouseEventHandler().event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    float changepos = wpt.x;

    const int sz = ctab_.size();
     if ( selidx_<0 || selidx_>=sz ) return;

     if ( changepos > 1 ) changepos = 1;
     if ( changepos < 0 ) changepos = 0;

     if ( !isSegmentized() )
     {
	 float position = mUdf(float); 
	 if ( (selidx_ > 0 && ctab_.position(selidx_-1)>=changepos) )
	     position = ctab_.position(selidx_-1) + 1.01*mEps;
	 else if ( (selidx_ < sz-1 && ctab_.position(selidx_+1)<=changepos) )
	     position = ctab_.position( selidx_+1 ) - 1.01*mEps;
	 else
	     position = changepos;
	 ctab_.changePos( selidx_, position );
	 update();
	 return;
     }

    int rightidx = mUdf(int);
    int leftidx = mUdf(int);
    float positionright = mUdf(float);
    float positionleft = mUdf(float);
    (selidx_%2)==0 ? rightidx = selidx_ : leftidx = selidx_;
    mIsUdf(rightidx) ? rightidx = leftidx+1 : leftidx = rightidx-1;
    if ( (leftidx > 0 && ctab_.position( leftidx-1 ) >= changepos) )
    {
	positionleft = ctab_.position( leftidx-1 ) + 1.01*mEps;
	positionright = positionleft + 0.9*mEps;
    }
    else if ( (rightidx < sz-1 && ctab_.position( rightidx+1 ) <= changepos) )
    {
	positionright = ctab_.position( rightidx+1 ) - 1.01*mEps;
	positionleft = positionright - 0.9*mEps;
    }
    else
    {
	positionright = changepos;
	positionleft = positionright - 0.9*mEps;
    }

    if ( positionright - positionleft > mEps )
	positionleft = positionright - 0.9*mEps;
    ctab_.changePos( leftidx, positionleft );
    ctab_.changePos( rightidx, positionright );

    update();
    getMouseEventHandler().setHandled( true );
    markerChanged.trigger();
}


bool uiColTabMarkerCanvas::isSegmentized()
{
    if ( ctab_.size() <= 3 )
	return false;

    for ( int idx=1; idx<ctab_.size()-2; idx+=2 )
    {
	if ( ctab_.position(idx+1)-ctab_.position(idx) > 1.01*mEps )
	    return false;
    }

    return true;
}
