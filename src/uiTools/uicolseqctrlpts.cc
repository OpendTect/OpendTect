/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/

#include "uicolseqctrlpts.h"

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


// TODO this is a hack because the painter is not using the full width
static const int cDroppedPixelsToRight = 4;

#define mSeqPosPerPix (1.0f / (scene().width()-cDroppedPixelsToRight))
#define mPixPerSeqPos ((float)(scene().width()-cDroppedPixelsToRight))
static const float cMaxSnapNrPix = 5.f;


static const int cColorCol = 1;

uiColSeqColCtrlPtsDlg::uiColSeqColCtrlPtsDlg( uiParent* p, Sequence& cseq )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage(
		tr("Color Control Points") ), tr("Manage Color Control Points"),
		mODHelpKey(mColTabMarkerDlgHelpID) ))
    , colseq_(cseq)
    , rollbackcseq_(new Sequence(cseq))
{
    uiTable::Setup tsu( colseq_.size(), 2 );
    tsu.rowgrow(true).rowdesc(tr("Control Point"))
	.defrowlbl(true).manualresize(true).removeselallowed(false);
    table_ = new uiTable( this, tsu, "Color Control Points table" );
    uiStringSet columnlabels;
    columnlabels.add(uiStrings::sPosition());
    columnlabels.add(uiStrings::sColor());
    table_->setColumnLabels( columnlabels );
    table_->setColumnReadOnly( cColorCol, true );
    fillTable();

    mAttachCB( table_->leftClicked, uiColSeqColCtrlPtsDlg::mouseClick );
    mAttachCB( table_->rowInserted, uiColSeqColCtrlPtsDlg::pointInserted );
    mAttachCB( table_->rowDeleted, uiColSeqColCtrlPtsDlg::pointDeleted );
    mAttachCB( table_->valueChanged, uiColSeqColCtrlPtsDlg::pointPosChgd );
    mAttachCB( colseq_.objectChanged(), uiColSeqColCtrlPtsDlg::seqChgCB );
}


void uiColSeqColCtrlPtsDlg::fillTable()
{
    NotifyStopper ns( table_->valueChanged );
    for ( int idx=0; idx<table_->nrCols(); idx++ )
    {
	MonitorLock ml( colseq_ );
	for ( int idy=0; idy<colseq_.size(); idy++ )
	{
	    RowCol rc;
	    rc.row() = idy;
	    rc.col() = idx;
	    const float position = colseq_.position( idy );
	    if ( rc.col() == 0 )
		table_->setValue( rc, 100.f * position );
	    if ( rc.col() == 1 )
	    {
		Color color( colseq_.color(position) );
		table_->setColor( rc, color );
	    }
	}
    }
}


void uiColSeqColCtrlPtsDlg::seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.isEntireObject()
      || chgdata.changeType() == Sequence::cColorChange() )
    {
	MonitorLock ml( colseq_ );
	table_->setNrRows( colseq_.size() );
	fillTable();
    }
}


void uiColSeqColCtrlPtsDlg::mouseClick( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rc = table_->notifiedCell();
    if ( rc.col() != cColorCol )
	return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,tr("Control Point color")) )
	colseq_.changeColor( rc.row(), newcol.r(), newcol.g(), newcol.b() );
}


void uiColSeqColCtrlPtsDlg::pointInserted( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcvalue = table_->newCell();
    if ( rcvalue.row()-1 < 0 || rcvalue.row() >= colseq_.size() )
    {
	table_->removeRow( rcvalue );
	uiMSG().error( tr("Cannot insert control points at the ends") );
	return;
    }

    RowCol rccolor( rcvalue.row(), 1 );
    const float newpos = colseq_.position(rcvalue.row()-1) +
			 ( colseq_.position(rcvalue.row()) -
			   colseq_.position(rcvalue.row()-1) ) / 2;
    Color col( colseq_.color(newpos) );
    colseq_.setColor( newpos, col.r(), col.g(), col.b() );
}


void uiColSeqColCtrlPtsDlg::pointDeleted( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    if ( rc.row() == 0 || rc.row() == colseq_.size()-1 )
    {
	uiMSG().error( tr("Cannot remove control points at the ends") );
	fillTable();
    }
    else
	colseq_.removeColor( rc.row() );
}


void uiColSeqColCtrlPtsDlg::pointPosChgd( CallBacker* )
{
    RowCol rc = table_->currentCell();
    const float newpos = table_->getFValue( rc ) * 0.01f;

    if ( colseq_.position(rc.row()-1)>newpos
      || colseq_.position(rc.row()+1)<newpos )
    {
	uiMSG().error( uiStrings::phrEnter(
		    tr("a position between surrounding Control Points")) );
	NotifyStopper notifstop( table_->valueChanged );
	table_->setValue( rc, colseq_.position(rc.row()) );
	return;
    }

    colseq_.changePos( rc.row(), newpos );
}


bool uiColSeqColCtrlPtsDlg::rejectOK()
{
    colseq_ = *rollbackcseq_;
    return true;
}


// ***** uiColSeqColCtrlPtsDisp ****

uiColSeqColCtrlPtsDisp::uiColSeqColCtrlPtsDisp( uiParent* p )
    : uiGraphicsView(p,"Color Control Points Canvas")
    , parent_(p)
    , markerlineitmgrp_(0)
    , meh_(scene().getMouseEventHandler())
    , colseq_(const_cast<ColTab::Sequence*>(
		    ColTab::SeqMGR().getDefault().ptr()))
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    curcptidx_ = -1;

    mAttachCB( reDrawNeeded, uiColSeqColCtrlPtsDisp::drawMarkers );
    mAttachCB( reSize, uiColSeqColCtrlPtsDisp::drawMarkers );

    mAttachCB( meh_.buttonPressed, uiColSeqColCtrlPtsDisp::mousePress );
    mAttachCB( meh_.movement, uiColSeqColCtrlPtsDisp::mouseMove );
    mAttachCB( meh_.buttonReleased, uiColSeqColCtrlPtsDisp::mouseRelease );
    mAttachCB( meh_.doubleClick, uiColSeqColCtrlPtsDisp::mouseDoubleClk );

    mAttachCB( colseq_->objectChanged(), uiColSeqColCtrlPtsDisp::seqChgCB );
}


void uiColSeqColCtrlPtsDisp::setSequence( Sequence& seq )
{
    if ( replaceMonitoredRef(colseq_,seq,this) )
	drawMarkers( 0 );
}


void uiColSeqColCtrlPtsDisp::drawMarkers( CallBacker* )
{
    if ( !colseq_ )
	return;

    scene().setSceneRect( 0, 0, mCast(float,width()), mCast(float,height()) );

    if ( !markerlineitmgrp_ )
    {
	markerlineitmgrp_ = new uiGraphicsItemGroup();
	scene().addItem( markerlineitmgrp_ );
    }
    else
	markerlineitmgrp_->removeAll( true );

    const int wdth = scene().width() - cDroppedPixelsToRight;
    const int hght = scene().height();
    MonitorLock ml( *colseq_ );
    for ( int idx=0; idx<colseq_->size(); idx++ )
    {
	const float val = colseq_->position( idx );
	uiLineItem* lineitem = new uiLineItem();
	const float fpos = (wdth-1) * val;
	int x = mNINT32( fpos );
	if ( x < 1 ) x = 1;
	if ( x > wdth-2 ) x = wdth - 2;
	lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,3) );
	lineitem->setLine( x, 0, x, hght-1 );
	markerlineitmgrp_->add( lineitem );
    }
}


void uiColSeqColCtrlPtsDisp::mousePress( CallBacker* cb )
{
    if ( !colseq_ || meh_.isHandled() )
	return;

    const MouseEvent& ev = meh_.event();
    float mindiff = cMaxSnapNrPix;
    curcptidx_ = -1;
    for ( int idx=0; idx<colseq_->size(); idx++ )
    {
	const float pixpos = mPixPerSeqPos * colseq_->position( idx );
	const float diffinpix = Math::Abs( ev.x() - pixpos );
	if ( diffinpix < mindiff )
	{
	    curcptidx_ = idx;
	    mindiff = diffinpix;
	}
    }

    if ( ev.buttonState() != OD::RightButton )
	return;

    uiMenu mnu( parent_, uiStrings::sAction() );
    if ( curcptidx_>=0 )
    {
	if ( curcptidx_ != 0 && curcptidx_ != colseq_->size()-1 )
	    mnu.insertItem( new uiAction(tr("Remove color")), 0 );
	mnu.insertItem( new uiAction(m3Dots(tr("Change color"))), 1 );
    }

    mnu.insertItem( new uiAction(m3Dots(tr("Edit in Table"))), 2 );

    const int res = mnu.exec();
    if ( res==0 )
	removeMarker( curcptidx_ );
    else if ( res==1 )
	changeColor( curcptidx_ );
    else if ( res==2 )
    {
	RefMan<Sequence> rollbackseq = new Sequence( *colseq_ );
	uiColSeqColCtrlPtsDlg dlg( parent_, *colseq_ );
	if ( !dlg.go() )
	    *colseq_ = *rollbackseq;
    }

    curcptidx_ = -1;
    meh_.setHandled( true );
}


void uiColSeqColCtrlPtsDisp::mouseMove( CallBacker* cb )
{
    NotifyStopper notifstop( meh_.buttonPressed );
    if ( !colseq_ || meh_.isHandled() )
	return;

    const int sz = colseq_->size();
    if ( curcptidx_<=0 || curcptidx_>=sz-1 )
	return;

    const MouseEvent& ev = meh_.event();
    const float evseqpos = mSeqPosPerPix * ev.x();

    float newcseqpos = evseqpos;
    const float prevmrkpos = colseq_->position( curcptidx_ - 1 );
    const float nextmrkpos = colseq_->position( curcptidx_ + 1 );
#   define mEps 0.00001f
    if ( evseqpos <= prevmrkpos )
	newcseqpos = prevmrkpos + mEps;
    else if ( evseqpos >= nextmrkpos )
	newcseqpos = nextmrkpos - mEps;

    colseq_->changePos( curcptidx_, newcseqpos );
    meh_.setHandled( true );
}


void uiColSeqColCtrlPtsDisp::mouseRelease( CallBacker* )
{
    curcptidx_ = -1;
    meh_.setHandled( true );
}


void uiColSeqColCtrlPtsDisp::mouseDoubleClk( CallBacker* cb )
{
    if ( !colseq_ || meh_.isHandled() )
	return;

    addMarker( mSeqPosPerPix*meh_.event().x(), true );
    curcptidx_ = -1;
    meh_.setHandled( true );
}


void uiColSeqColCtrlPtsDisp::seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.isEntireObject()
      || chgdata.changeType() == Sequence::cColorChange() )
	reDrawNeeded.trigger();
}


void uiColSeqColCtrlPtsDisp::addMarker( float pos, bool withcolsel )
{
    if ( !colseq_ )
	return;

    RefMan<Sequence> rollbackcseq = new Sequence( *colseq_ );;

    const Color col = colseq_->color( pos );
    const int cptidx = colseq_->setColor( pos, col.r(), col.g(), col.b() );

    if ( withcolsel )
    {
	if ( !changeColor( cptidx ) )
	    *colseq_ = *rollbackcseq;
    }
}


void uiColSeqColCtrlPtsDisp::removeMarker( int cptidx )
{
    if ( !colseq_ )
	return;

    colseq_->removeColor( cptidx );
    reDrawNeeded.trigger();
}


bool uiColSeqColCtrlPtsDisp::changeColor( int cptidx )
{
    if ( !colseq_ )
	return false;

    Color col = colseq_->color( colseq_->position(cptidx) );
    if ( !selectColor(col,parent_,tr("Selection color"),false) )
	return false;

    colseq_->changeColor( cptidx, col.r(), col.g(), col.b() );
    return true;
}
