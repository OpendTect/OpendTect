/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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


namespace PreStackView
{

uiViewer2DPosDlg::uiViewer2DPosDlg( uiParent* p, bool is2d,
	const CubeSampling& cs, const BufferStringSet& gathernms,
	bool issynthetic )
    : uiDialog(p,uiDialog::Setup("Pre-stack Gather display positions",
				0,"50.2.3").modal(false))
    , okpushed_(this)
    , is2d_(is2d)		     
{
    uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD :
	cs.defaultDir()==CubeSampling::Inl ? uiSliceSel::Inl : uiSliceSel::Crl;
    setCtrlStyle( DoAndStay );
    
    sliceselfld_ = new uiGatherPosSliceSel( this, tp, gathernms, issynthetic );
    sliceselfld_->enableScrollButton( false );
    if ( is2d_ || issynthetic )
	sliceselfld_->setMaxCubeSampling( cs );
    const bool isinl = tp == uiSliceSel::Inl;
    CubeSampling slicecs = cs;
    StepInterval<int> trcrg = is2d || isinl ? cs.hrg.crlRange()
					    : cs.hrg.inlRange();
    if ( is2d_ || isinl )
	slicecs.hrg.setCrlRange( trcrg );
    else
	slicecs.hrg.setInlRange( trcrg );
    setCubeSampling( slicecs );
    setOkText( "&Apply" );
}


bool uiViewer2DPosDlg::acceptOK( CallBacker* )
{
    okpushed_.trigger();
    return false;
}


void uiViewer2DPosDlg::getCubeSampling( CubeSampling& cs ) 
{
    sliceselfld_->acceptOK();
    cs = sliceselfld_->getCubeSampling();
    const int step = sliceselfld_->step();
    cs.hrg.step = BinID( step, step );
    if ( is2d_ )
	cs.hrg.setInlRange( Interval<int>( 1, 1 ) );
}


void uiViewer2DPosDlg::setCubeSampling( const CubeSampling& cs )
{
    const bool isinl = cs.defaultDir()==CubeSampling::Inl;
    const int step = is2d_ || isinl ? cs.hrg.crlRange().step
				    : cs.hrg.inlRange().step;
    sliceselfld_->setStep( step );
    sliceselfld_->setCubeSampling(cs);
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
    BufferString msg( "Dynamic " ); 
    const char* linetxt = isinl_ ? is2d_ ? "Trace" : "Xline" : "Inline";
    msg += linetxt;
    msg += is2d_ ? " Number" : " Range";
    if ( issynthetic )
    {
	inl0fld_->display( false, true );
	inl1fld_->display( false, true );
	crl0fld_->label()->setText( "Model range" );
    }

    stepfld_ = new uiLabeledSpinBox( this, "step" );
    stepfld_->attach( rightTo, isinl_ || is2d_ ? crl1fld_ : inl1fld_ ); 
    stepfld_->box()->setValue( 1 );

    updbut_ =
	new uiPushButton( this, "Update Postions",
			  mCB(this,uiGatherPosSliceSel,updatePosTable), true );
    updbut_->attach( alignedBelow, z0fld_ );


    uiSeparator* sep2 = new uiSeparator( this, "nr viewer/table sep" );
    sep2->attach( stretchedBelow, updbut_ );
    
    posseltbl_ =
	new uiTable( this, uiTable::Setup(10,gathernms_.size()),"Select");
    posseltbl_->attach( ensureBelow, sep2 );

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

    posseltbl_->setColumnLabels( gathernms_ );
    StepInterval<int> trcrg = is2d_ || isinl_ ? cs_.hrg.crlRange()
					      : cs_.hrg.inlRange();
    trcrg.step = stepfld_->box()->getValue();
    const int nrrows = trcrg.nrSteps()+1;
    posseltbl_->setNrRows( nrrows );

    BufferString lbl( issynthetic_ ? "Model"
	    			   : is2d_ ? "Trace" : isinl_ ? "CrossLine"
							      : "Inline" );
    lbl += " Nr ";
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
	    const int gatherpos = is2d_ || isinl_ ? ginfo.bid_.crl
						  : ginfo.bid_.inl;
	    const RowCol rc( rowidx, colidx );
	    CubeSampling cs( true );
	    const int limitstep =
		issynthetic_ ? 1 : isinl_ || is2d_ ? cs.hrg.crlRange().step
						   : cs.hrg.inlRange().step;
	    StepInterval<int> limitrg( issynthetic_ ? 1 : trcrg.start ,
		    		       trcrg.stop, limitstep );
	    uiGenInput* inpfld =
		new uiGenInput( 0, lbl, IntInpSpec(trcrg.atIndex(rowidx))
						   .setLimits(limitrg) );
	    inpfld->setWithCheck( true );
	    inpfld->setChecked( gatherinfos_[gatheridx].isselected_ );
	    inpfld->setValue( issynthetic_ ? dispgatheridxs_[idx]+1
		    			   : gatherpos );
	    inpfld->checked.notify(mCB(this,uiGatherPosSliceSel,gatherChecked));
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


void uiGatherPosSliceSel::setCubeSampling( const CubeSampling& cs )
{
    cs_ = cs;
    if ( issynthetic_ )
	cs_.hrg.setCrlRange( StepInterval<int>(0,gatherinfos_.size(),
		    			       stepfld_->box()->getValue()) );
    uiSliceSel::setCubeSampling( cs );
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
	int& pos = isinl_ ? ginfo.bid_.crl : ginfo.bid_.inl;
	pos = geninp->getIntValue();
    }
    else
    {
	GatherInfo ginfo = gatherinfos_[prevgatheridx];
	int selpos = geninp->getIntValue()-1;
    
	if ( issynthetic_ )
	{
	    CubeSampling cs( true );
	    StepInterval<int> trcrg( mUdf(int), -mUdf(int),
		    		     cs.hrg.crlRange().step );
	    for ( int idx=0; idx<gatherinfos_.size(); idx++ )
		trcrg.include( gatherinfos_[idx].bid_.crl, false );
	    selpos = trcrg.atIndex( selpos );
	}

	ginfo.bid_.crl = selpos;

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
    CubeSampling cs( true );

    StepInterval<int> trcrg( mUdf(int), -mUdf(int), issynthetic_
	    ? 1 : isinl_ || is2d_ ? cs.hrg.crlRange().step
	    			  : cs.hrg.inlRange().step );
    BufferString gnm = gatherinfos[0].gathernm_;
    int rgstep = mUdf(int);
    int prevginfoidx = mUdf(int);
    for ( int gidx=0; gidx<gatherinfos.size(); gidx++ )
    {
	const GatherInfo& ginfo = gatherinfos[gidx];
	const int trcnr = issynthetic_ ? gidx+1
	    			       : isinl_ || is2d_ ? ginfo.bid_.crl
							 : ginfo.bid_.inl;
	if ( !issynthetic_ || ginfo.isselected_  )
	{
	    if ( !mIsUdf(prevginfoidx) )
	    {
		const GatherInfo& prevginfo = gatherinfos[prevginfoidx];
		const int prevtrcnr =
		    issynthetic_ ? prevginfoidx+1
				 : isinl_ || is2d_ ? prevginfo.bid_.crl
						   : prevginfo.bid_.inl;
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

    trcrg.step = stepfld_->box()->getValue();
    if ( is2d_ || isinl_ )
	cs_.hrg.setCrlRange( trcrg );
    else
	 cs_.hrg.setInlRange( trcrg );

    if ( issynthetic_ )
	setMaxCubeSampling( cs_ );
    setCubeSampling( cs_ );
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
{ return stepfld_->box()->getValue(); }


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

    StepInterval<int> trcrg = is2d_ || isinl_ ? cs_.hrg.crlRange()
					      : cs_.hrg.inlRange();
    CubeSampling cs( true );
    trcrg.step = stepfld_->box()->getValue();
    for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
    {
	BufferString gathernm = gathernms_.get( colidx );
	for ( int rowidx=0; rowidx<trcrg.nrSteps()+1; rowidx++ )
	{
	    const RowCol rc( rowidx, colidx );
	    const int trcnr = trcrg.atIndex( rowidx );

	    if ( !issynthetic_ )
	    {
		BinID bid( isinl_ ? cs_.hrg.start.inl : trcnr,
			   isinl_ ? trcnr : cs_.hrg.start.crl );
		GatherInfo ginfo;
		ginfo.bid_ = bid;
		ginfo.gathernm_ = gathernm;
		ginfo.isstored_ = !issynthetic_;

		gatherinfos_ += ginfo;
		dispgatheridxs_ += gatherinfos_.size()-1;
	    }
	    else if ( gatherinfos_.validIdx(trcnr) )
		dispgatheridxs_ += trcnr-1;
	}
    }
}



uiViewer2DSelDataDlg::uiViewer2DSelDataDlg( uiParent* p, 
					    const BufferStringSet& gnms, 
						  BufferStringSet& selgnms )
    : uiDialog(p,uiDialog::Setup("Select gather data",
				"Add PS Gather","50.2.4"))
    , selgathers_(selgnms)
{
    allgatherfld_ = new uiListBox( this, "Available gathers", true );
    selgatherfld_ = new uiListBox( this, "Selected gathers", true );

    allgatherfld_->addItems( gnms );
    selgatherfld_->addItems( selgnms );

    uiLabel* sellbl = new uiLabel( this, "Select" );
    CallBack cb = mCB(this,uiViewer2DSelDataDlg,selButPush);
    toselect_ = new uiToolButton( this, uiToolButton::RightArrow,
				"Move right", cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->attach( centeredRightOf, allgatherfld_ );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( this, uiToolButton::LeftArrow,
	    			"Move left", cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selgatherfld_->attach( centeredRightOf, toselect_ );
}


void uiViewer2DSelDataDlg::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	for ( int idx=allgatherfld_->size()-1; idx>=0; idx-- )
	{
	    if ( !allgatherfld_->isSelected(idx) ) continue;
	    const char* txt = allgatherfld_->textOfItem(idx);
	    if ( selgatherfld_->isPresent( txt ) ) continue;
	    selgatherfld_->addItem( allgatherfld_->textOfItem(idx));
	    allgatherfld_->removeItem(idx);
	}
    }
    else if ( but == fromselect_ )
    {
	for ( int idx=selgatherfld_->size()-1; idx>=0; idx-- )
	{
	    if ( !selgatherfld_->isSelected(idx) ) continue;
	    const char* txt = selgatherfld_->textOfItem(idx);
	    if ( allgatherfld_->isPresent( txt ) ) continue;

	    allgatherfld_->addItem( txt );
	    selgatherfld_->removeItem(idx);
	    allgatherfld_->setSelected( allgatherfld_->size()-1 );
	}
    }
    allgatherfld_->sortItems();
    selgatherfld_->sortItems();
}


bool uiViewer2DSelDataDlg::acceptOK( CallBacker* )
{
    if ( selgatherfld_->isEmpty() )
    {
	uiMSG().error( "Please select at least one dataset" );
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
