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

#include "bufstring.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabsequence.h"
#include "draw.h"
#include "mouseevent.h"
#include "rowcol.h"
#include "od_helpids.h"

#include <math.h>


static const int sPosCol = 0;
static const int sColorCol = 1;

#define mEps 0.00001

uiColTabMarkerDlg::uiColTabMarkerDlg( uiParent* p, ColTab::Sequence& ctab )
    : uiDialog(p,Setup(uiStrings::phrManage(uiStrings::sAnchor(mPlural)),
		       tr("Add, Remove, and Edit Anchors"),
		       mODHelpKey(mColTabMarkerDlgHelpID)))
    , markersChanged(this)
    , ctab_(ctab)
{
    table_ = new uiTable( this, uiTable::Setup(ctab_.size(),2)
						.rowgrow(true)
						.rowdesc(uiStrings::sAnchor())
						.defrowlbl(true)
						.manualresize(true)
						.removeselallowed(false),
			  "Anchor Table");
    uiStringSet columnlabels;
    columnlabels.add( uiStrings::sPosition() )
		.add( uiStrings::sColor() );
    table_->setColumnLabels( columnlabels );
    table_->setColumnReadOnly( sColorCol, true );
    table_->setSelectionMode( uiTable::SingleRow );
    fillTable();

    mAttachCB( table_->leftClicked, uiColTabMarkerDlg::mouseClickCB );
    mAttachCB( table_->rowInserted, uiColTabMarkerDlg::markerInsertedCB );
    mAttachCB( table_->rowDeleted, uiColTabMarkerDlg::markerDeletedCB );
    mAttachCB( table_->valueChanged, uiColTabMarkerDlg::markerPosChgdCB );
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

    table_->setCellReadOnly( RowCol(0,0), true );
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,0), true );
}


void uiColTabMarkerDlg::mouseClickCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    if ( rc.col() != sColorCol )
	return;

    OD::Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,tr("Select Anchor Color")) )
    {
	ColTab::Sequence orgctab = ctab_;
	table_->setColor( rc, newcol );
	table_->setCurrentCell( RowCol(rc.row(),sPosCol) );
	orgctab.changeColor( rc.row(), newcol );
	ctab_ = orgctab;
    }

    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerInsertedCB( CallBacker* )
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


void uiColTabMarkerDlg::markerDeletedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    if ( rc.row()==0 || rc.row()==ctab_.size()-1 )
    {
	table_->insertRows( rc, 1 );
	fillTable();
	uiMSG().error( tr("Cannot remove anchors at extreme positions") );
	return;
    }

    ctab_.removeColor( rc.row() );
    fillTable();
    markersChanged.trigger();
}


void uiColTabMarkerDlg::markerPosChgdCB( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    if ( rc.row()<=0 || rc.row()>=ctab_.size()-1 )
	return;

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
    : uiGraphicsView(p,"Anchor Canvas")
    , parent_(p)
    , ctab_(ctab)
    , markerChanged(this)
    , meh_(scene().getMouseEventHandler())
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    selidx_ = mUdf(int);
    w2ui_ = new uiWorld2Ui( uiWorldRect(0,1,255,0),
			    uiSize(viewWidth(),viewHeight()) );

    mAttachCB( reDrawn, uiColTabMarkerCanvas::drawMarkersCB );

    mAttachCB( meh_.buttonPressed, uiColTabMarkerCanvas::mouseClkCB );
    mAttachCB( meh_.movement, uiColTabMarkerCanvas::mouseMoveCB );
    mAttachCB( meh_.doubleClick, uiColTabMarkerCanvas::mouseDoubleClickCB );
    mAttachCB( meh_.buttonReleased, uiColTabMarkerCanvas::mouseReleaseCB );
}


uiColTabMarkerCanvas::uiColTabMarkerCanvas( uiParent* p, ColTab::Sequence& ctab,
					    const Interval<float> surveyrg )
    : uiGraphicsView(p,"Anchor Canvas")
    , parent_(p)
    , ctab_(ctab)
    , markerChanged(this)
    , meh_(scene().getMouseEventHandler())
    , ctabrange_(surveyrg)
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    selidx_ = mUdf(int);
    w2ui_ = new uiWorld2Ui( uiWorldRect(0,1,255,0),
			    uiSize(viewWidth(),viewHeight()) );

    mAttachCB( reDrawn, uiColTabMarkerCanvas::drawMarkersCB );

    mAttachCB( meh_.buttonPressed, uiColTabMarkerCanvas::mouseClkCB );
    mAttachCB( meh_.movement, uiColTabMarkerCanvas::mouseMoveCB );
    mAttachCB( meh_.doubleClick, uiColTabMarkerCanvas::mouseDoubleClickCB );
    mAttachCB( meh_.buttonReleased, uiColTabMarkerCanvas::mouseReleaseCB );
}


uiColTabMarkerCanvas::~uiColTabMarkerCanvas()
{
    delete w2ui_;
}


void uiColTabMarkerCanvas::drawMarkersCB( CallBacker* )
{
    const int w = viewWidth();
    const int h = viewHeight();
    scene().setSceneRect( sCast(float,w), sCast(float,h), 0, 0 );
    w2ui_->set( uiRect(0,0,w,h), uiWorldRect(0,1,255,0) );

    int decimals = 2;
    char format = 'f';
    if ( !ctabrange_.isUdf() )
    {
	const float totalrange = std::fabs( ctabrange_.width() );

	if ( totalrange>=100000.0f || totalrange<0.2f )
	{
	    decimals = 3;
	    format = 'g';
	}
	else if ( totalrange < 20.0f )
	{
	    decimals = 2;
	    format = 'f';
	}
	else
	{
	    decimals = 0;
	    format = 'f';
	}
    }

    if ( !markerlineitmgrp_ )
    {
	markerlineitmgrp_ = new uiGraphicsItemGroup();
	scene().addItem( markerlineitmgrp_ );
    }
    else
	markerlineitmgrp_->removeAll( true );

    if ( ctab_.nrSegments() < 2 )
    {
	for ( int idx=0; idx<ctab_.size(); idx++ )
	{
	    const float val = ctab_.position(idx);
	    uiWorldPoint wpt( 0, val );
	    const uiPoint pt( w2ui_->transform(wpt) );
	    auto* lineitem = new uiLineItem();
	    lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,2) );
	    lineitem->setPenColor( OD::Color::Anthracite() );
	    if ( idx == 0 )
		lineitem->setLine( 0, pt.y_-2, w, pt.y_-2 );
	    else if ( idx == ctab_.size()-1 )
		lineitem->setLine( 0, pt.y_+2, w, pt.y_+2 );
	    else
		lineitem->setLine( 0, pt.y_, w, pt.y_ );

	    auto* txtitem = new uiTextItem();
	    txtitem->setTextColor( OD::Color::Anthracite() );
	    if ( ctabrange_.isUdf() )
	    {
		txtitem->setText( toUiString(val,0,format,decimals) );
	    }
	    else
	    {
		const float min = ctabrange_.start_;
		const float max = ctabrange_.stop_;
		float dataval = val*(max-min)+min;
		txtitem->setText( toUiString(dataval,0,format,decimals) );
	    }

	    if ( val==0 )
		txtitem->setAlignment( Alignment(Alignment::HCenter,
						 Alignment::Bottom) );
	    else
		txtitem->setAlignment( Alignment(Alignment::HCenter,
						 Alignment::Top) );

	    txtitem->setPos( uiPoint(w/2,pt.y_) );
	    txtitem->setVisible( true );
	    markerlineitmgrp_->add( txtitem );
	    markerlineitmgrp_->add( lineitem );
	}
    }
    else
    {
	const float nrseg = ctab_.nrSegments();
	const float segdist = 1.0f/nrseg;
	for ( int idx=0; idx<=nrseg; idx++ )
	{
	    const float val = idx*segdist;
	    uiWorldPoint wpt( 0, val );
	    const uiPoint pt( w2ui_->transform(wpt) );
	    auto* lineitem = new uiLineItem();
	    lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,2) );
	    lineitem->setPenColor( OD::Color::Anthracite() );

	    if ( idx == 0 )
		lineitem->setLine( 0, pt.y_-2, w, pt.y_-2 );
	    else if ( idx == nrseg )
		lineitem->setLine( 0, pt.y_+2, w, pt.y_+2 );
	    else
		lineitem->setLine( 0, pt.y_, w, pt.y_ );

	    auto* txtitem = new uiTextItem();
	    txtitem->setTextColor( OD::Color::Anthracite() );
	    if ( ctabrange_.isUdf() )
	    {
		txtitem->setText( toUiString(val,0,format,decimals) );
	    }
	    else
	    {
		const float min = ctabrange_.start_;
		const float max = ctabrange_.stop_;
		float dataval = val*(max-min)+min;
		txtitem->setText( toUiString(dataval,0,format,decimals) );
	    }

	    if ( idx == 0 )
		txtitem->setAlignment( Alignment(Alignment::HCenter,
						 Alignment::Bottom) );
	    else
		txtitem->setAlignment( Alignment(Alignment::HCenter,
						 Alignment::Top) );

	    txtitem->setPos( uiPoint(w/2,pt.y_) );
	    markerlineitmgrp_->add( txtitem );
	    markerlineitmgrp_->add( lineitem );
	}
    }
}


void uiColTabMarkerCanvas::eraseMarkersCB( CallBacker* )
{
    const int w = viewWidth();
    const int h = viewHeight();
    scene().setSceneRect( sCast(float,w), sCast(float,h), 0, 0 );
    w2ui_->set( uiRect(0,0,w,h), uiWorldRect(0,1,255,0) );

    if ( !markerlineitmgrp_ )
    {
	markerlineitmgrp_ = new uiGraphicsItemGroup();
	scene().addItem( markerlineitmgrp_ );
    }
    else
	markerlineitmgrp_->removeAll( true );
}


void uiColTabMarkerCanvas::mouseClkCB( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );

    selidx_ = -1;
    float mindiff = 5;
    uiWorldPoint wpp = w2ui_->worldPerPixel();
    float fac = (float)wpp.y_;
    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
	const float val = ctab_.position( idx );
	const float ref = (float) ( wpt.y_ );
	const float diffinpix = fabs(val-ref) / fabs(fac);
	if ( diffinpix < mindiff )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( OD::RightButton != ev.buttonState() )
	return;

    if ( markerlineitmgrp_->isEmpty() )
	return;

    uiMenu mnu( parent_, uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrEdit(
			 uiStrings::sAnchor(mPlural)))), 2 );

    if ( selidx_>=0 )
    {
	if ( selidx_ != 0 && selidx_ != ctab_.size()-1 )
	    mnu.insertAction( new uiAction(uiStrings::phrRemove(
				 uiStrings::sAnchor())), 0 );

	mnu.insertAction( new uiAction(m3Dots(tr("Change Color"))), 1 );
    }

    const int res = mnu.exec();
    if ( res==0 )
	removeMarker( selidx_ );
    else if ( res==1 )
	changeColor( selidx_ );
    else if ( res==2 )
    {
	ColTab::Sequence coltab = ctab_;
	//this will ensure that the color table manager will not be gray
	uiParent* dlgparent = parent_ ? parent_->mainwin()->parent() : parent_;
	uiColTabMarkerDlg dlg( dlgparent, ctab_ );
	mAttachCB( dlg.markersChanged, uiColTabMarkerCanvas::markerChgdCB );
	if ( !dlg.go() )
	{
	    ctab_ = coltab;
	    markerChgdCB( nullptr );
	}
    }

    selidx_ = -1;
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::markerChgdCB( CallBacker* )
{
    markerChanged.trigger();

    if ( !markerlineitmgrp_->isEmpty() )
	reDrawNeeded.trigger();
}

void uiColTabMarkerCanvas::setRange( const Interval<float> rg )
{
    ctabrange_ = rg;
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

    if ( !markerlineitmgrp_->isEmpty() )
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
    if ( !selectColor(col,parent_,tr("Anchor Color Selection"),false) )
	return false;

    ctab_.changeColor( markeridx, col.r(), col.g(), col.b() );
    markerChanged.trigger();
    return true;
}


void uiColTabMarkerCanvas::mouseDoubleClickCB( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    if ( ctab_.nrSegments()>1 )
	return;

    const MouseEvent& ev = meh_.event();
    const uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    addMarker( sCast(float,wpt.y_), true );
    selidx_ = -1;
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::mouseReleaseCB( CallBacker* )
{
    if ( meh_.isHandled() )
	return;

    if ( !markerlineitmgrp_->isEmpty() )
	reDrawNeeded.trigger();

    selidx_ = -1;
    reDrawNeeded.trigger();
    meh_.setHandled( true );
}


void uiColTabMarkerCanvas::mouseMoveCB( CallBacker* )
{
    NotifyStopper notifstop( meh_.buttonPressed );
    if ( meh_.isHandled() )
	return;

    if ( selidx_<=0 || selidx_==ctab_.size()-1 )
	return;

    const MouseEvent& ev = meh_.event();
    uiWorldPoint wpt = w2ui_->transform( ev.pos() );
    //float changepos = (float) ( wpt.y_ );

    const int sz = ctab_.size();
    if ( selidx_<0 || selidx_>=sz )
	return;

    const float changepos = std::clamp( (float)( wpt.y_ ), 0.0f, 1.0f );

    float position = mUdf(float);
    if ( (selidx_ > 0 && ctab_.position(selidx_-1)>=changepos) )
	position = (float) ( ctab_.position(selidx_-1) + 1.01*mEps );
    else if ( (selidx_ < sz-1 && ctab_.position(selidx_+1)<=changepos) )
	position = (float) ( ctab_.position( selidx_+1 ) - 1.01*mEps );
    else
	position = changepos;

    ctab_.changePos( selidx_, position );
    if ( !markerlineitmgrp_->isEmpty() )
	reDrawNeeded.trigger();
    markerChanged.trigger();
    meh_.setHandled( true );
}
