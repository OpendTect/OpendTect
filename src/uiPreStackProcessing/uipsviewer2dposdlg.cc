/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewer2dposdlg.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uislicesel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "od_helpids.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "zdomain.h"


namespace PreStackView
{
GatherInfo::GatherInfo()
{}


GatherInfo::~GatherInfo()
{}



uiViewer2DPosDlg::uiViewer2DPosDlg( uiParent* p, const TrcKeyZSampling& tkzs,
				    const BufferStringSet& gathernms )
    : uiDialog(p,uiDialog::Setup(tr("Prestack Gather display positions"),
			      mNoDlgTitle, mODHelpKey(mViewer2DPSPosDlgHelpID) )
			.modal(false))
    , okpushed_(this)
{
    setCtrlStyle( RunAndClose );

    const uiSliceSel::Type tp = uiSliceSel::getType( tkzs );
    sliceselfld_ = new uiGatherPosSliceSel( this, tp,
					    tkzs.hsamp_.getGeomID(), gathernms);
    sliceselfld_->enableScrollButton( false );
    if ( sliceselfld_->is2DSlice() )
	sliceselfld_->setMaxTrcKeyZSampling( tkzs );

    TrcKeyZSampling slicecs = tkzs;
    StepInterval<int> trcrg = sliceselfld_->useTrcNr() ? tkzs.hsamp_.trcRange()
						       : tkzs.hsamp_.inlRange();
    if ( sliceselfld_->useTrcNr() )
	slicecs.hsamp_.setCrlRange( trcrg );
    else
	slicecs.hsamp_.setInlRange( trcrg );

    setTrcKeyZSampling( slicecs );
    setOkText( uiStrings::sApply() );
}


uiViewer2DPosDlg::~uiViewer2DPosDlg()
{}


bool uiViewer2DPosDlg::acceptOK( CallBacker* )
{
    okpushed_.trigger();
    return false;
}


void uiViewer2DPosDlg::getTrcKeyZSampling( TrcKeyZSampling& cs )
{
    sliceselfld_->acceptOK();
    cs = sliceselfld_->getTrcKeyZSampling();
    const int step = sliceselfld_->step();
    cs.hsamp_.step_ = BinID( step, step );
}


void uiViewer2DPosDlg::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    const int step = sliceselfld_->useTrcNr() ? cs.hsamp_.crlRange().step
					      : cs.hsamp_.inlRange().step;
    sliceselfld_->setStep( step );
    sliceselfld_->setTrcKeyZSampling( cs );
}


// uiGatherPosSliceSel

uiGatherPosSliceSel::uiGatherPosSliceSel( uiParent* p, uiSliceSel::Type tp,
					  const Pos::GeomID& gid,
					  const BufferStringSet& gnms )
    : uiSliceSel(p,tp,ZDomain::SI(),gid)
    , gathernms_(gnms)
{
    setStretch( 2, 2 );
    uiString msg = tr( "Dynamic %1 %2" );
    const bool usetrc = is2DSlice();
    const char* linetxt = usetrc ? "Trace" : (isInl() ? "Inline" : "Xline");
    msg.arg(linetxt);
    msg.arg(usetrc ? tr(" Number") : tr(" Range"));
    if ( isSynth() )
	crl0fld_->label()->setText( tr("Model range") );

    stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep() );
    stepfld_->attach( rightTo, isInl() || usetrc ? crl1fld_ : inl1fld_ );
    stepfld_->box()->setValue( 1 );

    updbut_ = new uiPushButton( this, tr("Update Postions"),
			  mCB(this,uiGatherPosSliceSel,updatePosTable), true );
    updbut_->attach( alignedBelow, z0fld_ );

    auto* sep2 = new uiSeparator( this, "nr viewer/table sep" );
    sep2->attach( stretchedBelow, updbut_ );

    uiTable::Setup tablesu( 10, gathernms_.size() );
    tablesu.selmode( uiTable::Multi );
    posseltbl_ = new uiTable( this, tablesu, "Select Position");
    posseltbl_->attach( ensureBelow, sep2 );
    mAttachCB( posseltbl_->selectionChanged,
	       uiGatherPosSliceSel::cellSelectedCB );

    CallBack cb( mCB(this,uiGatherPosSliceSel,posChged) );
    stepfld_->box()->valueChanging.notify( cb );
    if ( inl0fld_ ) inl0fld_->box()->valueChanging.notify( cb );
    if ( crl0fld_ ) crl0fld_->box()->valueChanging.notify( cb );
    if ( inl1fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );
}


uiGatherPosSliceSel::~uiGatherPosSliceSel()
{
    detachAllNotifiers();
}


void uiGatherPosSliceSel::reDoTable()
{
    if ( !posseltbl_ )
	return;

    disptblposs_.erase();
    disptblposs_.setSize( dispgatheridxs_.size() );

    for ( int row=0; row<posseltbl_->nrRows(); row++ )
    {
	for ( int col=0; col<posseltbl_->nrCols(); col++ )
	    posseltbl_->clearCellObject( RowCol(row,col) );
    }

    posseltbl_->setColumnLabels( gathernms_.getUiStringSet() );
    StepInterval<int> trcrg = useTrcNr() ? tkzs_.hsamp_.trcRange()
					 : tkzs_.hsamp_.inlRange();
    trcrg.step = stepfld_->box()->getIntValue();
    const int nrrows = trcrg.nrSteps()+1;
    posseltbl_->setNrRows( nrrows );

    uiString lbl = tr("%1 Nr").arg(isSynth() ? tr("Model")
			      : is2DSlice() ? uiStrings::sTrace()
					    :(isInl() ? uiStrings::sCrossline()
						      : uiStrings::sInline()));
    for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
    {
	int rowidx = 0;
	BufferString curgnm = gathernms_.get( colidx );
	for ( int idx=0; idx<dispgatheridxs_.size(); idx++ )
	{
	    const int gatheridx = dispgatheridxs_[idx];
	    if ( !gatherinfos_.validIdx(gatheridx) )
		continue;

	    const GatherInfo& ginfo = gatherinfos_[gatheridx];
	    if ( ginfo.gathernm_ != curgnm )
		continue;

	    const int gatherpos = useTrcNr() ? ginfo.tk_.trcNr()
					     : ginfo.tk_.inl();
	    TrcKeyZSampling cs( ginfo.tk_.geomID() );
	    const RowCol rc( rowidx, colidx );
	    const int limitstep = useTrcNr() ? cs.hsamp_.trcRange().step
					     : cs.hsamp_.inlRange().step;
	    const StepInterval<int> limitrg( trcrg.start, trcrg.stop,limitstep);
	    auto* inpfld = new uiGenInput( nullptr, lbl,
					   IntInpSpec(trcrg.atIndex(rowidx))
							.setLimits(limitrg) );
	    inpfld->setWithCheck( true );
	    inpfld->setChecked( gatherinfos_[gatheridx].isselected_ );
	    inpfld->setValue( gatherpos );
	    mAttachCB( inpfld->checked, uiGatherPosSliceSel::gatherChecked );
	    mAttachCB( inpfld->valueChanging,
		       uiGatherPosSliceSel::gatherPosChanged );
	    inpfld->preFinalize().trigger();
	    posseltbl_->setCellGroup( rc, inpfld );
	    disptblposs_[idx] = rc;

	    rowidx++;
	}
    }

    updbut_->setSensitive( false );
}


void uiGatherPosSliceSel::cellSelectedCB( CallBacker* cb )
{
    TypeSet<RowCol> selectedcells;
    posseltbl_->getSelectedCells( selectedcells );
    for ( int cellidx=0; cellidx<selectedcells.size(); cellidx++ )
    {
	uiGroup* selectedgrp = posseltbl_->getCellGroup(selectedcells[cellidx]);
	mDynamicCastGet(uiGenInput*,selgi,selectedgrp);
	if ( !selgi )
	    continue;

	selgi->setChecked( true );
    }
}


void uiGatherPosSliceSel::gatherChecked( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,geninp,cb);
    if ( !geninp )
	return;

    const RowCol rc = posseltbl_->getCell( geninp );
    const int dispidx = disptblposs_.indexOf( rc );
    if ( dispidx < 0 )
	return;

    const int gatheridx = dispgatheridxs_[dispidx];
    if ( gatheridx < 0 )
	return;

    gatherinfos_[gatheridx].isselected_ = geninp->isChecked();
}


void uiGatherPosSliceSel::gatherPosChanged( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,geninp,cb);
    if ( !geninp )
	return;

    const RowCol rc = posseltbl_->getCell( geninp );
    const int dispidx = disptblposs_.indexOf( rc );
    if ( dispidx < 0 )
	return;

    const int prevgatheridx = dispgatheridxs_[dispidx];
    if ( prevgatheridx < 0 )
	return;

    GatherInfo& ginfo = gatherinfos_[prevgatheridx];
    int selpos = geninp->getIntValue();
    if ( isSynth() )
    {
	selpos--;
	StepInterval<int> trcrg( mUdf(int), -mUdf(int), SI().crlRange().step );
	for ( const auto& aginfo : gatherinfos_ )
	    trcrg.include( aginfo.tk_.trcNr(), false );

	selpos = trcrg.atIndex( selpos );
	ginfo.tk_.setTrcNr( selpos );
	const int curgatheridx = gatherinfos_.indexOf( ginfo );
	if ( curgatheridx >= 0 )
	{
	    dispgatheridxs_[dispidx] = curgatheridx;
	    gatherinfos_[prevgatheridx].isselected_ = false;
	    gatherinfos_[curgatheridx].isselected_ = geninp->isChecked();
	}
    }
    else
    {
	if ( is2DSlice() )
	    ginfo.tk_.setTrcNr( selpos );
	else
	{
	    if ( isInl() )
		ginfo.tk_.setCrl( selpos );
	    else
		ginfo.tk_.setInl( selpos );
	}
    }
}


void uiGatherPosSliceSel::setSelGatherInfos(
				const TypeSet<GatherInfo>& gatherinfos )
{
    gathernms_.erase();
    gatherinfos_ = gatherinfos;
    TrcKeyZSampling cs( tkzs_.hsamp_.getGeomID() );

    StepInterval<int> trcrg( mUdf(int), -mUdf(int),
			    useTrcNr() ? maxcs_.hsamp_.trcRange().step
				       : maxcs_.hsamp_.inlRange().step );
    BufferString firstgnm = gatherinfos[0].gathernm_;
    int rgstep = mUdf(int);
    int prevginfoidx = mUdf(int);
    for ( int gidx=0; gidx<gatherinfos.size(); gidx++ )
    {
	const GatherInfo& ginfo = gatherinfos[gidx];
	const int trcnr = useTrcNr() ? ginfo.tk_.trcNr()
				     : ginfo.tk_.inl();
	if ( ginfo.isselected_ && ginfo.gathernm_ == firstgnm )
	{
	    if ( !mIsUdf(prevginfoidx) )
	    {
		const GatherInfo& prevginfo = gatherinfos[prevginfoidx];
		const int prevtrcnr = useTrcNr() ? prevginfo.tk_.trcNr()
						 : prevginfo.tk_.inl();
		if ( abs(trcnr-prevtrcnr) < rgstep )
		    rgstep = abs(trcnr-prevtrcnr);
	    }

	    prevginfoidx = gidx;
	}

	trcrg.include( trcnr, false );
	gathernms_.addIfNew( ginfo.gathernm_ );
    }

    rgstep = trcrg.snapStep( rgstep );

    StepInterval<int> steprg( trcrg.step, trcrg.width(), trcrg.step );
    stepfld_->box()->setInterval( steprg );
    stepfld_->box()->setValue( rgstep );

    trcrg.step = stepfld_->box()->getIntValue();
    if ( useTrcNr() )
	tkzs_.hsamp_.setTrcRange( trcrg );
    else
	tkzs_.hsamp_.setInlRange( trcrg );

    setTrcKeyZSampling( tkzs_ );

    dispgatheridxs_.setEmpty();
    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
    {
	if ( gatherinfos_[idx].isselected_ )
	    dispgatheridxs_ += idx;
    }

    reDoTable();
}


void uiGatherPosSliceSel::getSelGatherInfos( TypeSet<GatherInfo>& gatherinfos )
{
    gatherinfos = gatherinfos_;
}


int uiGatherPosSliceSel::step() const
{
    return stepfld_->box()->getIntValue();
}


void uiGatherPosSliceSel::setStep( int ns )
{
    StepInterval<int> steprg = stepfld_->box()->getInterval();
    ns = steprg.snapStep( ns );
    stepfld_->box()->setValue(ns);
}


void uiGatherPosSliceSel::enableZDisplay( bool yn )
{
    z0fld_->display( yn );
    z1fld_->display( yn );
}


void uiGatherPosSliceSel::posChged( CallBacker* cb )
{
    updbut_->setSensitive( true );
}


void uiGatherPosSliceSel::updatePosTable( CallBacker* )
{
    acceptOK();
    if ( isSynth() )
    {
	for ( auto& ginfo : gatherinfos_ )
	    ginfo.isselected_ = false;
    }
    else
	gatherinfos_.erase();

    resetDispGatherInfos();
    reDoTable();
}


void uiGatherPosSliceSel::resetDispGatherInfos()
{
    dispgatheridxs_.erase();
    if ( !isSynth() )
	gatherinfos_.erase();

    StepInterval<int> trcrg = useTrcNr() ? tkzs_.hsamp_.crlRange()
					 : tkzs_.hsamp_.inlRange();
    TrcKeyZSampling cs( tkzs_.hsamp_.getGeomID() );
    trcrg.step = stepfld_->box()->getIntValue();
    for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
    {
	BufferString gathernm = gathernms_.get( colidx );
	for ( int rowidx=0; rowidx<trcrg.nrSteps()+1; rowidx++ )
	{
	    const RowCol rc( rowidx, colidx );
	    const int trcnr = trcrg.atIndex( rowidx );

	    if ( isSynth() )
	    {
		const int ginfoidx = trcnr-1 + ((trcrg.width()+1)*colidx);
		if ( !gatherinfos_.validIdx(ginfoidx) )
		    continue;

		dispgatheridxs_ += ginfoidx;
		gatherinfos_[ginfoidx].isselected_ = true;
	    }
	    else
	    {
		TrcKey tk;
		if ( is2D() )
		    tk.setGeomID( tkzs_.hsamp_.getGeomID() ).setTrcNr( trcnr );
		else
		{
		    tk.setIs3D()
		      .setInl( isInl() ? tkzs_.hsamp_.start_.inl() : trcnr )
		      .setCrl( isInl() ? trcnr : tkzs_.hsamp_.start_.crl() );
		}

		GatherInfo ginfo;
		ginfo.tk_ = tk;
		ginfo.gathernm_ = gathernm;
		ginfo.isstored_ = true;
		ginfo.isselected_ = true;

		gatherinfos_ += ginfo;
		dispgatheridxs_ += gatherinfos_.size()-1;
	    }
	}
    }
}


// uiViewer2DSelDataDlg

uiViewer2DSelDataDlg::uiViewer2DSelDataDlg( uiParent* p,
					    const BufferStringSet& gnms,
						  BufferStringSet& selgnms )
    : uiDialog(p,uiDialog::Setup(tr("Select gather data"),
				 tr("Add PS Gather"),
				 mODHelpKey(mViewer2DPSSelDataDlgHelpID) ))
    , selgathers_(selgnms)
{
    allgatherfld_ = new uiListBox( this, "Available gathers",
				   OD::ChooseAtLeastOne );
    selgatherfld_ = new uiListBox( this, "Selected gathers",
				   OD::ChooseAtLeastOne );

    allgatherfld_->addItems( gnms );
    selgatherfld_->addItems( selgnms );

    auto* sellbl = new uiLabel( this, uiStrings::sSelect() );
    CallBack cb = mCB(this,uiViewer2DSelDataDlg,selButPush);
    toselect_ = new uiToolButton( this, uiToolButton::RightArrow,
				  tr("Move right"), cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->attach( centeredRightOf, allgatherfld_ );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( this, uiToolButton::LeftArrow,
				    tr("Move left"), cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selgatherfld_->attach( centeredRightOf, toselect_ );
}


uiViewer2DSelDataDlg::~uiViewer2DSelDataDlg()
{}


void uiViewer2DSelDataDlg::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    uiListBox* fromfld = but == toselect_ ? allgatherfld_ : selgatherfld_;
    uiListBox* tofld = but == toselect_ ? selgatherfld_ : allgatherfld_;

    BufferStringSet removegathernms;
    for ( int idx=0; idx<fromfld->size(); idx++ )
    {
	if ( !fromfld->isChosen(idx) )
	    continue;

	removegathernms.addIfNew( fromfld->textOfItem(idx) );
    }

    while ( !removegathernms.isEmpty() )
    {
	BufferString remgathernm = removegathernms.get( 0 );
	removegathernms.removeSingle( 0 );
	tofld->addItem( toUiString(remgathernm.buf()) );
	const int remidx = fromfld->indexOf( remgathernm.buf() );
	fromfld->removeItem( remidx );
    }

    allgatherfld_->sortItems();
    allgatherfld_->chooseAll( false );
    selgatherfld_->sortItems();
    selgatherfld_->chooseAll();
}


bool uiViewer2DSelDataDlg::acceptOK( CallBacker* )
{
    if ( selgatherfld_->isEmpty() )
    {
	uiMSG().error( tr("Please select at least one dataset") );
	return false;
    }

    selgathers_.setEmpty();
    for ( int idx=0; idx<selgatherfld_->size(); idx++ )
    {
	const char* txt = selgatherfld_->textOfItem( idx );
	selgathers_.addIfNew( txt );
    }
    return true;
}

} // namespace PreStackView
