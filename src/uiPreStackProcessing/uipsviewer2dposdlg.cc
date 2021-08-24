/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
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

#include "seisioobjinfo.h"
#include "zdomain.h"
#include "od_helpids.h"


namespace PreStackView
{

uiViewer2DPosDlg::uiViewer2DPosDlg( uiParent* p, bool is2d,
	const TrcKeyZSampling& cs, const BufferStringSet& gathernms,
	bool issynthetic )
    : uiDialog(p,uiDialog::Setup(tr("Prestack Gather display positions"),
			      mNoDlgTitle, mODHelpKey(mViewer2DPSPosDlgHelpID) )
                              .modal(false))
    , okpushed_(this)
    , is2d_(is2d)
{
    uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD :
	cs.defaultDir()==TrcKeyZSampling::Inl ? uiSliceSel::Inl
					      : uiSliceSel::Crl;
    setCtrlStyle( RunAndClose );

    sliceselfld_ = new uiGatherPosSliceSel( this, tp, gathernms, issynthetic );
    sliceselfld_->enableScrollButton( false );
    if ( is2d_ || issynthetic )
	sliceselfld_->setMaxTrcKeyZSampling( cs );
    const bool isinl = tp == uiSliceSel::Inl;
    TrcKeyZSampling slicecs = cs;
    StepInterval<int> trcrg = is2d || isinl ? cs.hsamp_.crlRange()
					    : cs.hsamp_.inlRange();
    if ( is2d_ || isinl )
	slicecs.hsamp_.setCrlRange( trcrg );
    else
	slicecs.hsamp_.setInlRange( trcrg );
    setTrcKeyZSampling( slicecs );
    setOkText( uiStrings::sApply() );
}


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
    if ( is2d_ )
	cs.hsamp_.setInlRange( Interval<int>( 1, 1 ) );
}


void uiViewer2DPosDlg::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    const bool isinl = cs.defaultDir()==TrcKeyZSampling::Inl;
    const int step = is2d_ || isinl ? cs.hsamp_.crlRange().step
				    : cs.hsamp_.inlRange().step;
    sliceselfld_->setStep( step );
    sliceselfld_->setTrcKeyZSampling(cs);
}


uiGatherPosSliceSel::uiGatherPosSliceSel( uiParent* p, uiSliceSel::Type tp,
					  const BufferStringSet& gnms,
					  bool issynthetic )
  : uiSliceSel(p,tp,ZDomain::SI(),!issynthetic)
  , posseltbl_(0)
  , gathernms_(gnms)
  , issynthetic_(issynthetic)
{
    setStretch( 2, 2 );
    uiString msg = tr( "Dynamic %1 %2" );
    const char* linetxt = isinl_ ? is2d_ ? "Trace" : "Xline" : "Inline";
    msg.arg(linetxt);
    msg.arg(is2d_ ? tr(" Number") : tr(" Range"));
    if ( issynthetic )
    {
	inl0fld_->display( false, true );
	inl1fld_->display( false, true );
	crl0fld_->label()->setText( tr("Model range") );
    }

    stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep() );
    stepfld_->attach( rightTo, isinl_ || is2d_ ? crl1fld_ : inl1fld_ );
    stepfld_->box()->setValue( 1 );

    updbut_ =
	new uiPushButton( this, tr("Update Postions"),
			  mCB(this,uiGatherPosSliceSel,updatePosTable), true );
    updbut_->attach( alignedBelow, z0fld_ );


    uiSeparator* sep2 = new uiSeparator( this, "nr viewer/table sep" );
    sep2->attach( stretchedBelow, updbut_ );

    uiTable::Setup tablesu( 10, gathernms_.size() );
    tablesu.selmode( uiTable::Multi );
    posseltbl_ =
	new uiTable( this, tablesu, "Select Position");
    posseltbl_->attach( ensureBelow, sep2 );
    posseltbl_->selectionChanged.notify(
	    mCB(this,uiGatherPosSliceSel,cellSelectedCB) );

    CallBack cb( mCB(this,uiGatherPosSliceSel,posChged) );
    stepfld_->box()->valueChanging.notify( cb );
    if ( inl0fld_ ) inl0fld_->box()->valueChanging.notify( cb );
    if ( crl0fld_ ) crl0fld_->box()->valueChanging.notify( cb );
    if ( inl1fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );
}


void uiGatherPosSliceSel::reDoTable()
{
    if ( !posseltbl_ ) return;
    disptblposs_.erase();
    disptblposs_.setSize( dispgatheridxs_.size() );

    for ( int row=0; row<posseltbl_->nrRows(); row++ )
    {
	for ( int col=0; col<posseltbl_->nrCols(); col++ )
	    posseltbl_->clearCellObject( RowCol(row,col) );
    }

    posseltbl_->setColumnLabels( gathernms_.getUiStringSet() );
    StepInterval<int> trcrg = is2d_ || isinl_ ? tkzs_.hsamp_.crlRange()
					      : tkzs_.hsamp_.inlRange();
    trcrg.step = stepfld_->box()->getIntValue();
    const int nrrows = trcrg.nrSteps()+1;
    posseltbl_->setNrRows( nrrows );

    uiString lbl = tr("%1 Nr").arg(issynthetic_ ? tr("Model")
			      : is2d_ ? uiStrings::sTrace() : isinl_ ?
			      uiStrings::sCrossline() : uiStrings::sInline() );
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
	    const int gatherpos = is2d_ || isinl_ ? ginfo.bid_.crl()
						  : ginfo.bid_.inl();
	    TrcKeyZSampling cs( true );
	    const int modelnr =
		(ginfo.bid_.crl()-cs.hsamp_.stop_.crl())/cs.hsamp_.step_.crl();
	    const RowCol rc( rowidx, colidx );
	    const int limitstep =
		issynthetic_ ? 1 : isinl_ || is2d_ ? cs.hsamp_.crlRange().step
						   : cs.hsamp_.inlRange().step;
	    StepInterval<int> limitrg( issynthetic_ ? 1 : trcrg.start ,
				       trcrg.stop, limitstep );
	    uiGenInput* inpfld =
		new uiGenInput( 0, lbl, IntInpSpec(trcrg.atIndex(rowidx))
						   .setLimits(limitrg) );
	    inpfld->setWithCheck( true );
	    inpfld->setChecked( gatherinfos_[gatheridx].isselected_ );
	    inpfld->setValue( issynthetic_ ? modelnr : gatherpos );
	    inpfld->checked.notify(mCB(this,uiGatherPosSliceSel,
                                       gatherChecked));
	    inpfld->valuechanging.notify(
		    mCB(this,uiGatherPosSliceSel,gatherPosChanged));
	    inpfld->preFinalise().trigger();
	    posseltbl_->setCellGroup( rc, inpfld );
	    disptblposs_[idx] = rc;

	    rowidx++;
	}
    }

    updbut_->setSensitive( false );
}


void uiGatherPosSliceSel::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    tkzs_ = cs;
    if ( issynthetic_ )
	tkzs_.hsamp_.setCrlRange( StepInterval<int>(0,gatherinfos_.size(),
					      stepfld_->box()->getIntValue()) );
    uiSliceSel::setTrcKeyZSampling( cs );
}


void uiGatherPosSliceSel::cellSelectedCB( CallBacker* cb )
{
    TypeSet<RowCol> selectedcells;
    posseltbl_->getSelectedCells( selectedcells );
    for ( int cellidx=0; cellidx<selectedcells.size(); cellidx++ )
    {
	uiGroup* selectedgrp = posseltbl_->getCellGroup(selectedcells[cellidx]);
	mDynamicCastGet(uiGenInput*,selgi,selectedgrp);
	if ( !selgi ) continue;

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
    if ( dispidx < 0 ) return;
    const int gatheridx = dispgatheridxs_[dispidx];
    if ( gatheridx < 0 ) return;

    gatherinfos_[gatheridx].isselected_ = geninp->isChecked();
}


void uiGatherPosSliceSel::gatherPosChanged( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,geninp,cb);
    if ( !geninp )
	return;
    const RowCol rc = posseltbl_->getCell( geninp );
    const int dispidx = disptblposs_.indexOf( rc );
    if ( dispidx < 0 ) return;
    const int prevgatheridx = dispgatheridxs_[dispidx];
    if ( prevgatheridx < 0 ) return;

    if ( !issynthetic_ )
    {
	GatherInfo& ginfo = gatherinfos_[prevgatheridx];
	int& pos = isinl_ ? ginfo.bid_.crl() : ginfo.bid_.inl();
	pos = geninp->getIntValue();
    }
    else
    {
	GatherInfo ginfo = gatherinfos_[prevgatheridx];
	int selpos = geninp->getIntValue()-1;

	if ( issynthetic_ )
	{
	    TrcKeyZSampling cs( true );
	    StepInterval<int> trcrg( mUdf(int), -mUdf(int),
				     cs.hsamp_.crlRange().step );
	    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
		trcrg.include( gatherinfos_[idx].bid_.crl(), false );
	    selpos = trcrg.atIndex( selpos );
	}

	ginfo.bid_.crl() = selpos;

	const int curgatheridx = gatherinfos_.indexOf( ginfo );
	if ( curgatheridx >= 0 )
	{
	    dispgatheridxs_[dispidx] = curgatheridx;
	    gatherinfos_[prevgatheridx].isselected_ = false;
	    gatherinfos_[curgatheridx].isselected_ = geninp->isChecked();
	}
    }
}


void uiGatherPosSliceSel::setSelGatherInfos(
	const TypeSet<GatherInfo>& gatherinfos )
{
    gathernms_.erase();
    gatherinfos_ = gatherinfos;
    TrcKeyZSampling cs( true );

    StepInterval<int> trcrg( mUdf(int), -mUdf(int), issynthetic_
	    ? 1 : isinl_ || is2d_ ? cs.hsamp_.crlRange().step
				  : cs.hsamp_.inlRange().step );
    BufferString firstgnm = gatherinfos[0].gathernm_;
    int rgstep = mUdf(int);
    int prevginfoidx = mUdf(int);
    int modelnr = 0;
    for ( int gidx=0; gidx<gatherinfos.size(); gidx++ )
    {
	const GatherInfo& ginfo = gatherinfos[gidx];
	if ( ginfo.gathernm_ == firstgnm )
	    modelnr++;

	const int trcnr = issynthetic_ ? modelnr
				       : isinl_ || is2d_ ? ginfo.bid_.crl()
							 : ginfo.bid_.inl();
	if ( (!issynthetic_ || ginfo.isselected_) &&
	     ginfo.gathernm_ == firstgnm )
	{
	    if ( !mIsUdf(prevginfoidx) )
	    {
		const GatherInfo& prevginfo = gatherinfos[prevginfoidx];
		const int prevtrcnr =
		    issynthetic_ ? prevginfoidx+1
				 : isinl_ || is2d_ ? prevginfo.bid_.crl()
						   : prevginfo.bid_.inl();
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
    if ( is2d_ || isinl_ )
	tkzs_.hsamp_.setCrlRange( trcrg );
    else
	 tkzs_.hsamp_.setInlRange( trcrg );

    if ( issynthetic_ )
	setMaxTrcKeyZSampling( tkzs_ );
    setTrcKeyZSampling( tkzs_ );
    dispgatheridxs_.erase();

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
{ return stepfld_->box()->getIntValue(); }


void uiGatherPosSliceSel::setStep( int ns )
{
    StepInterval<int> steprg = stepfld_->box()->getInterval();
    ns = steprg.snapStep( ns );
    stepfld_->box()->setValue(ns);
}


void uiGatherPosSliceSel::enableZDisplay( bool yn )
{ z0fld_->display( yn ); z1fld_->display( yn ); }


void uiGatherPosSliceSel::posChged( CallBacker* cb )
{ updbut_->setSensitive( true ); }


void uiGatherPosSliceSel::updatePosTable( CallBacker* )
{
    acceptOK();
    if ( !issynthetic_ )
	gatherinfos_.erase();
    else
    {
	for ( int idx=0; idx<gatherinfos_.size(); idx++ )
	    gatherinfos_[idx].isselected_ = false;
    }

    resetDispGatherInfos();
    reDoTable();
}


void uiGatherPosSliceSel::resetDispGatherInfos()
{
    dispgatheridxs_.erase();
    if ( !issynthetic_ )
	gatherinfos_.erase();

    StepInterval<int> trcrg = is2d_ || isinl_ ? tkzs_.hsamp_.crlRange()
					      : tkzs_.hsamp_.inlRange();
    TrcKeyZSampling cs( true );
    trcrg.step = stepfld_->box()->getIntValue();
    for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
    {
	BufferString gathernm = gathernms_.get( colidx );
	for ( int rowidx=0; rowidx<trcrg.nrSteps()+1; rowidx++ )
	{
	    const RowCol rc( rowidx, colidx );
	    const int trcnr = trcrg.atIndex( rowidx );

	    if ( !issynthetic_ )
	    {
		BinID bid( isinl_ ? tkzs_.hsamp_.start_.inl() : trcnr,
			   isinl_ ? trcnr : tkzs_.hsamp_.start_.crl() );
		GatherInfo ginfo;
		ginfo.bid_ = bid;
		ginfo.gathernm_ = gathernm;
		ginfo.isstored_ = !issynthetic_;
		ginfo.isselected_ = true;

		gatherinfos_ += ginfo;
		dispgatheridxs_ += gatherinfos_.size()-1;
	    }
	    else
	    {
		const int ginfoidx = trcnr-1 + ((trcrg.width()+1)*colidx);
		if ( !gatherinfos_.validIdx(ginfoidx) )
		    continue;
		dispgatheridxs_ += ginfoidx;
		gatherinfos_[ginfoidx].isselected_ = true;
	    }
	}
    }
}



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

    uiLabel* sellbl = new uiLabel( this, uiStrings::sSelect() );
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
    selgathers_.erase();
    for ( int idx=0; idx<selgatherfld_->size(); idx++ )
    {
	const char* txt = selgatherfld_->textOfItem( idx );
	selgathers_.addIfNew( txt );
    }
    return true;
}

} //namespace
