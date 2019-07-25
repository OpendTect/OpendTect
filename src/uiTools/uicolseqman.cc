/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          February 2008
________________________________________________________________________

-*/

#include "uicolseqman.h"

#include "bufstring.h"
#include "coltabseqmgr.h"
#include "draw.h"
#include "datadistributiontools.h"
#include "mouseevent.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolseqdisp.h"
#include "uicoltabsel.h"
#include "uicolseqimport.h"
#include "uichecklist.h"
#include "uifunctiondisplay.h"
#include "uirgbarray.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitreeview.h"
#include "uiworld2ui.h"
#include "uistrings.h"
#include "od_helpids.h"

static const int cTranspDispHeight = 180;
static const int cTranspDispWidth = GetGoldenMajor( cTranspDispHeight );
static const int cCtrlDispHeight = cTranspDispHeight / 7;
static const int cSeqDispHeight = cTranspDispHeight / 5;


static const int cColorCol = 1;

class uiColSeqColCtrlPtsDlg : public uiDialog
{ mODTextTranslationClass(uiColSeqColCtrlPtsDlg);
public:

    typedef ColTab::Sequence	Sequence;

uiColSeqColCtrlPtsDlg( uiParent* p, Sequence& cseq )
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

    mAttachCB( table_->colorSelectionChanged, uiColSeqColCtrlPtsDlg::colChgCB );
    mAttachCB( table_->rowInserted, uiColSeqColCtrlPtsDlg::pointInserted );
    mAttachCB( table_->rowDeleted, uiColSeqColCtrlPtsDlg::pointDeleted );
    mAttachCB( table_->valueChanged, uiColSeqColCtrlPtsDlg::pointPosChgd );
    mAttachCB( colseq_.objectChanged(), uiColSeqColCtrlPtsDlg::seqChgCB );
}

void fillTable()
{
    NotifyStopper ns( table_->valueChanged, this );
    for ( int idx=0; idx<table_->nrCols(); idx++ )
    {
	MonitorLock ml( colseq_ );
	for ( int idy=0; idy<colseq_.size(); idy++ )
	{
	    RowCol rc; rc.row() = idy; rc.col() = idx;
	    const float position = colseq_.position( idy );
	    if ( rc.col() == 0 )
		table_->setValue( rc, 100.f * position );
	    if ( rc.col() == 1 )
	    {
		table_->setColorSelectionCell( rc, false );
		table_->setCellColor( rc, colseq_.color(position) );
	    }
	}
    }
}

void seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.isEntireObject()
      || chgdata.changeType() == Sequence::cColorChange() )
    {
	NotifyStopper nsvc( table_->valueChanged, this );
	NotifyStopper nsri( table_->rowInserted, this );
	NotifyStopper nsrd( table_->rowDeleted, this );
	MonitorLock ml( colseq_ );
	table_->setNrRows( colseq_.size() );
	fillTable();
    }
}

void colChgCB( CallBacker* cb )
{
    mCBCapsuleUnpack( RowCol, rc, cb );
    const Color newcol( table_->getCellColor(rc) );
    colseq_.changeColor( rc.row(), newcol.r(), newcol.g(), newcol.b() );
}

void pointInserted( CallBacker* )
{
    NotifyStopper nsvc( table_->valueChanged, this );
    RowCol rcvalue = table_->newCell();
    if ( rcvalue.row()-1 < 0 || rcvalue.row() >= colseq_.size() )
    {
	NotifyStopper nsrd( table_->rowDeleted, this );
	table_->removeRow( rcvalue );
	uiMSG().error( tr("Cannot insert control points at the ends") );
	return;
    }
    table_->setColorSelectionCell( rcvalue, false );

    RowCol rccolor( rcvalue.row(), 1 );
    const float newpos = colseq_.position(rcvalue.row()-1) +
			 ( colseq_.position(rcvalue.row()) -
			   colseq_.position(rcvalue.row()-1) ) / 2;
    Color col( colseq_.color(newpos) );
    NotifyStopper ns( colseq_.objectChanged(), this );
    colseq_.setColor( newpos, col.r(), col.g(), col.b() );
}

void pointDeleted( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    if ( rc.row() == 0 || rc.row() == colseq_.size()-1 )
    {
	uiMSG().error( tr("Cannot remove control points at the ends") );
	fillTable();
    }
    else
    {
	NotifyStopper ns( colseq_.objectChanged(), this );
	colseq_.removeColor( rc.row() );
    }
}

void pointPosChgd( CallBacker* )
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

    NotifyStopper ns( colseq_.objectChanged(), this );
    colseq_.changePos( rc.row(), newpos );
}

bool rejectOK()
{
    NotifyStopper ns( colseq_.objectChanged(), this );
    colseq_ = *rollbackcseq_;
    return true;
}

    uiTable*		table_;
    Sequence&		colseq_;
    ConstRefMan<Sequence> rollbackcseq_;

};


#define mSeqPosPerPix ((float)(1.0 / scene().maxX()))
#define mPixPerSeqPos ((float)scene().maxX())
static const float cMaxSnapNrPix = 5.f;


class uiColSeqColCtrlPtsEd : public uiGraphicsView
{ mODTextTranslationClass(uiColSeqColCtrlPtsEd);
public:

    typedef ColTab::Sequence	Sequence;

uiColSeqColCtrlPtsEd( uiGroup* p, uiColSeqMan* csm )
    : uiGraphicsView(p,"Color Control Points Canvas")
    , uiseqman_(csm)
    , markerlineitmgrp_(0)
    , meh_(scene().getMouseEventHandler())
{
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    curcptidx_ = -1;

    mAttachCB( reDrawNeeded, uiColSeqColCtrlPtsEd::drawMarkers );
    mAttachCB( reSize, uiColSeqColCtrlPtsEd::drawMarkers );

    mAttachCB( meh_.buttonPressed, uiColSeqColCtrlPtsEd::mousePress );
    mAttachCB( meh_.movement, uiColSeqColCtrlPtsEd::mouseMove );
    mAttachCB( meh_.buttonReleased, uiColSeqColCtrlPtsEd::mouseRelease );
    mAttachCB( meh_.doubleClick, uiColSeqColCtrlPtsEd::mouseDoubleClk );

    mAttachCB( colseq().objectChanged(), uiColSeqColCtrlPtsEd::seqChgCB );
}

void drawMarkers( CallBacker* )
{
    if ( markerlineitmgrp_ )
	markerlineitmgrp_->removeAll( true );
    else
    {
	markerlineitmgrp_ = new uiGraphicsItemGroup;
	scene().addItem( markerlineitmgrp_ );
    }

    const int xmax = scene().nrPixX() - 1;
    const int ymax = scene().nrPixY() - 1;

    uiLineItem* lineitem = new uiLineItem;
    lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,2) );
    lineitem->setPenColor( Color(0,255,255) );
    lineitem->setLine( 1, 0, 1, ymax );
    lineitem->setZValue( 10 );
    markerlineitmgrp_->add( lineitem );
    lineitem = new uiLineItem;
    lineitem->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,2) );
    lineitem->setPenColor( Color(0,255,255) );
    lineitem->setLine( xmax-1, ymax, xmax-1, ymax );
    lineitem->setZValue( 10 );
    markerlineitmgrp_->add( lineitem );

    uiManipHandleItem::Setup msu;
    msu.thickness_ = 2;
    MonitorLock ml( colseq() );
    for ( int idx=0; idx<colseq().size(); idx++ )
    {
	const float fpos = xmax * colseq().position( idx );
	uiManipHandleItem* itm = new uiManipHandleItem( msu );
	markerlineitmgrp_->add( itm );
	itm->setPixPos( fpos );
    }
}

void mousePress( CallBacker* cb )
{
    if ( meh_.isHandled() )
	return;

    const MouseEvent& ev = meh_.event();
    float mindiff = cMaxSnapNrPix;
    curcptidx_ = -1;
    for ( int idx=0; idx<colseq().size(); idx++ )
    {
	const float pixpos = mPixPerSeqPos * colseq().position( idx );
	const float diffinpix = Math::Abs( ev.x() - pixpos );
	if ( diffinpix < mindiff )
	{
	    curcptidx_ = idx;
	    mindiff = diffinpix;
	}
    }

    if ( ev.buttonState() != OD::RightButton )
	return;

    uiMenu mnu( uiseqman_, uiStrings::sAction() );
    if ( curcptidx_>=0 )
    {
	if ( curcptidx_ != 0 && curcptidx_ != colseq().size()-1 )
	    mnu.insertAction( new uiAction(tr("Remove color")), 0 );
	mnu.insertAction( new uiAction(m3Dots(tr("Change color"))), 1 );
    }

    mnu.insertAction( new uiAction(m3Dots(tr("Edit in Table"))), 2 );

    const int res = mnu.exec();
    if ( res==0 )
	removeMarker( curcptidx_ );
    else if ( res==1 )
	changeColor( curcptidx_ );
    else if ( res==2 )
    {
	uiColSeqColCtrlPtsDlg dlg( uiseqman_, colseq() );
	dlg.go();
    }

    curcptidx_ = -1;
    meh_.setHandled( true );
}

void mouseMove( CallBacker* cb )
{
    if ( meh_.isHandled() )
	return;

    const int sz = colseq().size();
    if ( curcptidx_<=0 || curcptidx_>=sz-1 )
	return;

    const MouseEvent& ev = meh_.event();
    const float evseqpos = mSeqPosPerPix * ev.x();

    float newcseqpos = evseqpos;
    const float prevmrkpos = colseq().position( curcptidx_ - 1 );
    const float nextmrkpos = colseq().position( curcptidx_ + 1 );
#   define mEps 0.00001f
    if ( evseqpos <= prevmrkpos )
	newcseqpos = prevmrkpos + mEps;
    else if ( evseqpos >= nextmrkpos )
	newcseqpos = nextmrkpos - mEps;

    colseq().changePos( curcptidx_, newcseqpos );
    meh_.setHandled( true );
}

void mouseRelease( CallBacker* )
{
    curcptidx_ = -1;
    meh_.setHandled( true );
}

void mouseDoubleClk( CallBacker* cb )
{
    if ( meh_.isHandled() )
	return;

    addCtrlPt( mSeqPosPerPix*meh_.event().x(), true );
    curcptidx_ = -1;
    meh_.setHandled( true );
}

void seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.isEntireObject()
      || chgdata.changeType() == Sequence::cColorChange() )
    {
	reDrawNeeded.trigger();
	drawMarkers( 0 );
    }
}

void addCtrlPt( float pos, bool withcolsel )
{
    RefMan<Sequence> rollbackcseq = new Sequence( colseq() );
    NotifyStopper ns( colseq().objectChanged(), this );

    const Color col = colseq().color( pos );
    const int cptidx = colseq().setColor( pos, col.r(), col.g(), col.b() );

    if ( withcolsel )
    {
	if ( !changeColor( cptidx ) )
	    colseq() = *rollbackcseq;
    }
    reDrawNeeded.trigger();
}

void removeMarker( int cptidx )
{
    colseq().removeColor( cptidx );
}

bool changeColor( int cptidx )
{
    Color col = colseq().color( colseq().position(cptidx) );
    if ( !selectColor(col,uiseqman_,tr("Selection color"),false) )
	return false;

    NotifyStopper ns( colseq().objectChanged(), this );
    colseq().changeColor( cptidx, col.r(), col.g(), col.b() );
    return true;
}

    uiColSeqMan*	uiseqman_;
    Sequence&		colseq()	    { return *uiseqman_->curseq_; }
    uiGraphicsItemGroup* markerlineitmgrp_;
    MouseEventHandler&	meh_;
    int			curcptidx_;

};


uiColSeqMan::uiColSeqMan( uiParent* p, const char* initialseqnm )
    : uiDialog(p,
	uiDialog::Setup(uiStrings::phrManage(uiStrings::sColorTable(mPlural)),
			 mNoDlgTitle,mODHelpKey(mColorTableManHelpID) ))
    , seqmgr_(ColTab::SeqMGR4Edit())
    , rollbackmgr_(ColTab::SeqMGR().clone())
    , selectionChanged(this)
{
    setModal( false );
    setShrinkAllowed( false );
    ConstRefMan<Sequence> initialcur = seqmgr_.getAny( initialseqnm );
    rollbackseq_ = new Sequence( *initialcur );
    curseq_ = seqmgr_.get4Edit( initialcur->name() );

    uiGroup* leftgrp = new uiGroup( this, "Left" );
    uiGroup* rightgrp = new uiGroup( this, "Right" );

    seqlistfld_ = new uiTreeView( leftgrp, "Color Table List" );
    BufferStringSet labels;
    labels.add( "Color table" ).add( "Status" );
    seqlistfld_->addColumns( labels );
    seqlistfld_->setRootDecorated( false );
    seqlistfld_->setHScrollBarMode( uiTreeView::AlwaysOff );
    seqlistfld_->setStretch( 2, 2 );
    seqlistfld_->setSelectionBehavior( uiTreeView::SelectRows );

    const OD::LineStyle ls( OD::LineStyle::Solid, 1, Color::LightGrey() );
    uiFunctionDisplay::Setup su;
    su	.editable( true )
	.xrg( Interval<float>( 0, 1 ) )
	.yrg( Interval<float>( 0, 255 ) )
	.canvaswidth( cTranspDispWidth ).canvasheight( cTranspDispHeight )
	.drawborder( true ).borderstyle( ls ).border( uiBorder(3,5,3,5) )
	.ycol( Color::Red() ).y2col( Color::LightGrey() )
	.drawscattery1( true ).closepolygon( true )
	.drawliney2( false ).fillbelowy2( true )
	.pointsz( 3 ).ptsnaptol( 0.08 )
	.noxaxis( true ).noyaxis( true ).noy2axis( true )
	.noxgridline( true ).noygridline( true ).noy2gridline( true );
    transpdisp_ = new uiFunctionDisplay( rightgrp, su );
    transpdisp_->setStretch( 2, 2 );
    RefMan<FloatDistrib> y2distr = uiCOLTAB().mapper().distribution().clone();
    const int distrsz = y2distr->size();
    if ( distrsz > 2 )
    {
	DataDistributionChanger<float> distrchgr( *y2distr );
	distrchgr.deSpike();
	y2distr->setSampling( SamplingData<float>(0.f,1.0f/(distrsz-1)) );
	distrchgr.normalise( false );
	transpdisp_->setY2Vals( *y2distr, true );
    }

    ctrlptsed_ = new uiColSeqColCtrlPtsEd( rightgrp, this );
    ctrlptsed_->setPrefWidth( cTranspDispWidth );
    ctrlptsed_->setPrefHeight( cCtrlDispHeight );
    ctrlptsed_->setStretch( 2, 2 );
    ctrlptsed_->attach( ensureBelow, transpdisp_ );

    seqdisp_ = new uiColSeqDisp( rightgrp, OD::Horizontal, false, false );
    seqdisp_->setPrefWidth( cTranspDispWidth );
    seqdisp_->setPrefHeight( cSeqDispHeight );
    seqdisp_->setSeqName( curseq_->name() );
    seqdisp_->attach( ensureBelow, ctrlptsed_, 0 );
    seqdisp_->setStretch( 2, 2 );

    uiStringSet segtypes;
    segtypes.add( uiStrings::sNone() );
    segtypes.add( uiStrings::sFixed() );
    segtypes.add( uiStrings::sVariable(false) );
    segtypefld_ = new uiGenInput( rightgrp, uiStrings::sSegmentation(),
				  StringListInpSpec(segtypes) );
    segtypefld_->attach( ensureBelow, seqdisp_ );
    nrsegfld_ = new uiLabeledSpinBox( rightgrp, tr("Number of segments"), 0 );
    nrsegfld_->box()->setInterval( 2, 64 ); nrsegfld_->box()->setValue( 8 );
    nrsegfld_->display( false );
    nrsegfld_->attach( alignedBelow, segtypefld_ );

    uiColorInput::Setup cisetup( curseq_->undefColor(),
				 uiColorInput::Setup::Separate );
    cisetup.lbltxt( tr("Undefined color") );
    undefcolfld_ = new uiColorInput( rightgrp, cisetup );
    undefcolfld_->attach( alignedBelow, nrsegfld_ );

    uiColorInput::Setup ctsu( curseq_->markColor(), uiColorInput::Setup::None );
    ctsu.withdesc( false );
    markcolfld_ = new uiColorInput( rightgrp,
                                      ctsu.lbltxt(tr("Marker color")) );
    markcolfld_->attach( alignedBelow, undefcolfld_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter" );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

    uiButton* impbut = uiButton::getStd( this, OD::Import,
				     mCB(this,uiColSeqMan,impColSeqCB), false );
    impbut->attach( ensureBelow, splitter );

    removebut_ = uiButton::getStd( this, OD::Remove,
				    mCB(this,uiColSeqMan,removeCB), false );
    removebut_->attach( centeredRightOf, impbut );

    // TODO button for disable? Menu?
	// mAttachCB( xx, uiColSeqMan::toggleDisabledCB );

    mAttachCB( postFinalise(), uiColSeqMan::doFinalise );
}


uiColSeqMan::~uiColSeqMan()
{
    detachAllNotifiers();
    ColTab::SequenceManager::deleteInst( rollbackmgr_ );
}


void uiColSeqMan::doFinalise( CallBacker* )
{
    updateColSeqList();
    handleSeqChg();
    toStatusBar( uiString::empty(), 1 );

    mAttachCB( seqlistfld_->selectionChanged, uiColSeqMan::selChgCB );
    mAttachCB( transpdisp_->pointSelected, uiColSeqMan::transpPtSelCB );
    mAttachCB( transpdisp_->pointChanged, uiColSeqMan::transpPtChgCB );
    mAttachCB( segtypefld_->valuechanged, uiColSeqMan::segmentTypeSelCB );
    mAttachCB( nrsegfld_->box()->valueChanging, uiColSeqMan::nrSegmentsChgCB );
    mAttachCB( undefcolfld_->colorChanged, uiColSeqMan::undefColSelCB );
    mAttachCB( markcolfld_->colorChanged, uiColSeqMan::markerColSelCB );

    mAttachCB( curseq_->objectChanged(), uiColSeqMan::seqChgCB );
    mAttachCB( seqmgr_.objectChanged(), uiColSeqMan::seqMgrChgCB );
}


void uiColSeqMan::updateColSeqList()
{
    BufferStringSet csnms;
    seqmgr_.getSequenceNames( csnms );
    csnms.sort();

    NotifyStopper ns( seqlistfld_->selectionChanged );
    seqlistfld_->clear();
    uiTreeViewItem* curitm = 0;
    for ( int idx=0; idx<csnms.size(); idx++ )
    {
	ConstRefMan<Sequence> seq = seqmgr_.getByName( csnms.get(idx) );
	if ( !seq )
	    continue;

	uiTreeViewItem* itm = new uiTreeViewItem( seqlistfld_,
		uiTreeViewItem::Setup()
		.label(toUiString(seq->name()))
		.label(seq->statusDispStr()) );

	if ( seq == curseq_ )
	    curitm = itm;
    }

    if ( curitm )
    {
	ns.enableNotification();
	seqlistfld_->setCurrentItem( curitm );
	seqlistfld_->setSelected( curitm, true );
	seqlistfld_->ensureItemVisible( curitm );
    }
}


void uiColSeqMan::selChgCB( CallBacker* )
{
    const uiTreeViewItem* itm = seqlistfld_->selectedItem();
    if ( !itm )
	{ pErrMsg("No sel item"); return; }

    RefMan<Sequence> newseq = seqmgr_.get4Edit( itm->text(0) );
    if ( !newseq || curseq_.ptr() == newseq.ptr() )
	return;

    if ( curseq_ )
	rollbackseq_ = newseq->clone();

    replaceMonitoredRef( curseq_, newseq, this );

    handleSeqChg();
    selectionChanged.trigger();
}


void uiColSeqMan::toggleDisabledCB( CallBacker* cb )
{
    curseq_->setDisabled( !curseq_->disabled() );
}


void uiColSeqMan::removeCB( CallBacker* )
{
    const BufferString csnm = curseq_->name();
    const Sequence::Status status = curseq_->status();
    uiString msg;
    if ( status != Sequence::Edited )
    {
	if ( status == Sequence::System )
	    msg = tr("Remove (hide) System Color Table %1?" );
	else
	    msg = tr("Remove Color Table %1?" );
	msg.arg( csnm );
	if ( !uiMSG().askRemove(msg) )
	    return;
    }
    else
    {
	uiDialog askdlg( this, uiDialog::Setup( tr("Please select"),
	    tr("Removing edited system color table - what action to take?"),
	    mNoHelpKey ) );
	uiCheckList* cl = new uiCheckList( &askdlg, uiCheckList::OneOnly );
	cl->addItem( tr("Reset '%1' to original system values").arg( csnm ) );
	cl->addItem( tr("Remove (hide) color table '%1'").arg( csnm ) );
	cl->addItem( tr("Cancel the operation") );
	if ( !askdlg.go() || cl->firstChecked() == 2 )
	    return;
	if ( cl->firstChecked() == 0 )
	{
	    ConstRefMan<Sequence> sysseq = seqmgr_.getSystemSeq( csnm );
	    if ( !sysseq )
		{ pErrMsg("Huh"); }
	    else
		*curseq_ = *sysseq;
	    return;
	}
    }

    seqmgr_.removeByName( csnm );
}


bool uiColSeqMan::save()
{
    uiRetVal uirv = seqmgr_.write();
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    return true;
}


bool uiColSeqMan::acceptOK()
{
    if ( seqmgr_.needsSave() )
	return save();
    return true;
}


bool uiColSeqMan::rejectOK()
{
    const bool havechanges = rollbackmgr_->dirtyCount() != seqmgr_.dirtyCount();
    if ( havechanges && !uiMSG().askGoOn( tr("Undo all changes?") ) )
	return false;

    stopReceivingNotifications();
    ctrlptsed_->stopReceivingNotifications();
    seqdisp_->stopReceivingNotifications();

    if ( havechanges )
	seqmgr_.rollbackFromCopy( *rollbackmgr_ );

    return true;
}


void uiColSeqMan::undefColSelCB( CallBacker* )
{
    curseq_->setUndefColor( undefcolfld_->color() );
}


void uiColSeqMan::markerColSelCB( CallBacker* )
{
    curseq_->setMarkColor( markcolfld_->color() );
}


void uiColSeqMan::segmentTypeSelCB( CallBacker* )
{
    nrsegfld_->display( segtypefld_->getIntValue()==1 );
    setSegmentation();
}


void uiColSeqMan::nrSegmentsChgCB( CallBacker* )
{
    setSegmentation();
}


void uiColSeqMan::setSegmentation()
{
    const int segtyp = segtypefld_->getIntValue();
    if ( segtyp == 0 )
	curseq_->setNrSegments( 0 );
    else if ( segtyp == 1 )
    {
	const int nrseg = nrsegfld_->box()->getIntValue();
	if ( !mIsUdf(nrseg) && nrseg > 1 )
	    curseq_->setNrSegments( nrseg );
	else
	    curseq_->setNrSegments( 0 );
    }
    else
	curseq_->setNrSegments( -1 );
}


void uiColSeqMan::seqMgrChgCB( CallBacker* cb )
{
    updateColSeqList();
}


void uiColSeqMan::seqChgCB( CallBacker* cb )
{
    mGetMonitoredChgDataWithCaller( cb, chgdata, caller );
    mDynamicCastGet( Sequence*, seq, caller );
    if ( !seq || chgdata.isNoChange() )
	return;

#   define mIsType(nm) (chgdata.changeType() == Sequence::nm())

    const bool isall = chgdata.isEntireObject();
    if ( isall || mIsType(cNameChange) )
	updateColSeqList();
    else
    {
	uiTreeViewItem* itm = seqlistfld_->findItem( seq->name(), 0, false );
	if ( !itm )
	    { pErrMsg("Huh"); }
	else
	{
	    itm->setText( toUiString(seq->name()), 0 );
	    itm->setText( seq->statusDispStr(), 1 );
	}
    }

    if ( seq == curseq_.ptr() )
    {
	if ( isall || mIsType(cTransparencyChange) )
	    updateTransparencyGraph();
	if ( isall || mIsType(cSegmentationChange) )
	    updateSegmentationFields();
	if ( isall || mIsType(cMarkColChange) || mIsType(cUdfColChange) )
	    updateSpecColFlds();
    }

}


void uiColSeqMan::handleSeqChg()
{
    seqdisp_->setSequence( *curseq_ );
    updateTransparencyGraph();
    updateSegmentationFields();
    updateSpecColFlds();
    updateActionStates();
}


void uiColSeqMan::updateTransparencyGraph()
{
    TypeSet<float> xvals;
    TypeSet<float> yvals;
    for ( int idx=0; idx<curseq_->transparencySize(); idx++ )
    {
	const Sequence::TranspPtType transp = curseq_->transparency( idx );
	xvals += transp.first;
	yvals += transp.second;
    }

    transpdisp_->setVals( xvals.arr(), yvals.arr(), xvals.size()  );
}


void uiColSeqMan::updateSegmentationFields()
{
    NotifyStopper ns( nrsegfld_->box()->valueChanging );
    int val = 0;
    if ( curseq_->isSegmentized() )
	val = curseq_->nrSegments()>0 ? 1 : 2;

    segtypefld_->setValue( val );
    nrsegfld_->display( val==1 );
    nrsegfld_->box()->setValue( val==1 ? curseq_->nrSegments() : 8 );
}


void uiColSeqMan::updateSpecColFlds()
{
    NotifyStopper nsudf( undefcolfld_->colorChanged );
    undefcolfld_->setColor( curseq_->undefColor() );
    NotifyStopper nsmrk( undefcolfld_->colorChanged );
    markcolfld_->setColor( curseq_->markColor() );
}


void uiColSeqMan::updateActionStates()
{
    removebut_->setSensitive( seqmgr_.size() > 0 );
}


void uiColSeqMan::transpPtChgCB( CallBacker* )
{
    const int ptidx = transpdisp_->selPt();
    const int nrpts = transpdisp_->xVals().size();
    if ( ptidx < 0 )
    {
	curseq_->removeTransparencies();
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    Geom::Point2D<float> pt( transpdisp_->xVals()[idx],
				     transpdisp_->yVals()[idx] );
	    if ( !pt.isDefined() )
		continue;

	    Sequence::TranspPtType tpt( pt.x_,
					mRounded(Sequence::CompType,pt.y_) );
	    if ( idx==0 )
		tpt.first = 0.f;
	    else if ( idx==nrpts-1 )
		tpt.first = 1.f;

	    curseq_->setTransparency( tpt );
	}
    }
    else
    {
	Geom::Point2D<float> pt( transpdisp_->xVals()[ptidx],
				 transpdisp_->yVals()[ptidx] );
	if ( !pt.isDefined() )
	    return;

	Sequence::TranspPtType tpt( pt.x_, mRounded(Sequence::CompType,pt.y_));

	const ColTab::PosType poseps = 0.00001f;
	bool reset = false;
	if ( ptidx==0 && !mIsZero(tpt.first,poseps) )
	{
	    tpt.first = 0.f;
	    reset = true;
	}
	else if ( ptidx==nrpts-1 && !mIsZero(tpt.first-1,poseps) )
	{
	    tpt.first = 1.f;
	    reset = true;
	}

	if ( reset )
	    updateTransparencyGraph();

	curseq_->changeTransparency( ptidx, tpt );
    }
}


void uiColSeqMan::transpPtSelCB( CallBacker* )
{
    const int ptidx = transpdisp_->selPt();
    const int nrpts = transpdisp_->xVals().size();
    if ( ptidx<0 || nrpts == curseq_->transparencySize() )
	return;

    Geom::Point2D<float> pt( transpdisp_->xVals()[ptidx],
			     transpdisp_->yVals()[ptidx] );
    if ( !pt.isDefined() )
	return;

    const Sequence::TranspPtType tpt( pt.x_,
				      mRounded(Sequence::CompType,pt.y_) );

    curseq_->setTransparency( tpt );
}


void uiColSeqMan::impColSeqCB( CallBacker* )
{
    uiColSeqImport dlg( this );
    dlg.go();
}
