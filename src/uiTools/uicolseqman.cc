/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          February 2008
________________________________________________________________________

-*/

#include "uicolseqman.h"

#include "bufstring.h"
#include "coltabsequence.h"
#include "draw.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "settings.h"
#include "uistring.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolseqdisp.h"
#include "uicolseqimport.h"
#include "uicolseqctrlpts.h"
#include "uichecklist.h"
#include "uifunctiondisplay.h"
#include "uirgbarray.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiworld2ui.h"
#include "uistrings.h"
#include "od_helpids.h"

static const int cTranspDispHeight = 150;
static const int cTranspDispWidth = GetGoldenMajor( cTranspDispHeight );
static const int cSeqDispHeight = cTranspDispHeight / 8;


uiColSeqMan::uiColSeqMan( uiParent* p, const char* initialseqnm )
    : uiDialog(p,
	uiDialog::Setup(uiStrings::phrManage(uiStrings::sColorTable(mPlural)),
			 mNoDlgTitle,mODHelpKey(mColorTableManHelpID) ))
    , seqmgr_(ColTab::SeqMGR4Edit())
    , distrib_(new DistribType)
    , rollbackmgr_(ColTab::SeqMGR().clone())
    , selectionChanged(this)
{
    setModal( false );
    ConstRefMan<Sequence> initialcur = seqmgr_.getAny( initialseqnm );
    rollbackseq_ = new Sequence( *initialcur );
    curseq_ = seqmgr_.get4Edit( initialcur->name() );

    setShrinkAllowed( false );

    uiGroup* leftgrp = new uiGroup( this, "Left" );

    seqlistfld_ = new uiTreeView( leftgrp, "Color Table List" );
    BufferStringSet labels;
    labels.add( "Color table" ).add( "Status" );
    seqlistfld_->addColumns( labels );
    seqlistfld_->setRootDecorated( false );
    seqlistfld_->setHScrollBarMode( uiTreeView::AlwaysOff );
    seqlistfld_->setStretch( 2, 2 );
    seqlistfld_->setSelectionBehavior( uiTreeView::SelectRows );

    uiGroup* rightgrp = new uiGroup( this, "Right" );

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
    transpdisp_->setStretch( 2, 0 );

    ctrlptsdisp_ = new uiColSeqColCtrlPtsDisp( rightgrp );
    ctrlptsdisp_->setPrefWidth( cTranspDispWidth );
    ctrlptsdisp_->setPrefHeight( cSeqDispHeight );
    ctrlptsdisp_->setStretch( 2, 0 );
    ctrlptsdisp_->attach( alignedBelow, transpdisp_ );

    seqdisp_ = new uiColSeqDisp( rightgrp );
    seqdisp_->setPrefWidth( cTranspDispWidth );
    seqdisp_->setPrefHeight( cSeqDispHeight );
    seqdisp_->setSeqName( curseq_->name() );
    seqdisp_->attach( alignedBelow, ctrlptsdisp_, 0 );
    seqdisp_->setStretch( 2, 0 );

    const char* segtypes[] = { "None", "Fixed", "Variable", 0 };
    segtypefld_ = new uiGenInput( rightgrp, tr("Segmentation"),
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

    uiPushButton* impbut = new uiPushButton( this, uiStrings::sImport(), false);
    impbut->attach( ensureBelow, splitter );
    mAttachCB( impbut->activated, uiColSeqMan::impColSeqCB );

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
    distrChgCB( 0 );
    toStatusBar( uiString::emptyString(), 1 );

    mAttachCB( seqlistfld_->selectionChanged, uiColSeqMan::selChgCB );
    mAttachCB( transpdisp_->pointSelected, uiColSeqMan::transpPtSelCB );
    mAttachCB( transpdisp_->pointChanged, uiColSeqMan::transpPtChgCB );
    mAttachCB( segtypefld_->valuechanged, uiColSeqMan::segmentTypeSelCB );
    mAttachCB( nrsegfld_->box()->valueChanging, uiColSeqMan::nrSegmentsChgCB );
    mAttachCB( undefcolfld_->colorChanged, uiColSeqMan::undefColSelCB );
    mAttachCB( markcolfld_->colorChanged, uiColSeqMan::markerColSelCB );
    mAttachCB( distrib_->objectChanged(), uiColSeqMan::distrChgCB );

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
	mAttachCBIfNotAttached( seq->objectChanged(), uiColSeqMan::seqChgCB );

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


void uiColSeqMan::setDistrib( const DistribType* dd )
{
    if ( replaceMonitoredRef(distrib_,dd,this) )
	distrChgCB( 0 );
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
    ctrlptsdisp_->stopReceivingNotifications();
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


void uiColSeqMan::distrChgCB( CallBacker* )
{
    if ( distrib_ )
    {
	RefMan<DataDistribution<float> > y2distr = distrib_->clone();
	const int sz = y2distr->size();
	y2distr->setSampling( SamplingData<float>(0.f,1.0f/(sz-1)) );
	transpdisp_->setY2Vals( *y2distr, true );
    }
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
    ctrlptsdisp_->setSequence( *curseq_ );
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
					mRounded(Sequence::ValueType,pt.y_) );
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

	Sequence::TranspPtType tpt( pt.x_, mRounded(Sequence::ValueType,pt.y_));

	const Sequence::PosType poseps = 0.00001f;
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
				      mRounded(Sequence::ValueType,pt.y_) );

    curseq_->setTransparency( tpt );
}


void uiColSeqMan::impColSeqCB( CallBacker* )
{
    uiColSeqImport dlg( this );
    dlg.go();
}
