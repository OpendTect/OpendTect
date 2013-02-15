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
#include "uicombobox.h"
#include "uilabel.h"
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


#define cStartNrVwrs 8

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
    trcrg.step = trcrg.width()/cStartNrVwrs;
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
    sliceselfld_->updatePosTable();
}


uiGatherPosSliceSel::uiGatherPosSliceSel( uiParent* p, uiSliceSel::Type tp,
					  const BufferStringSet& gnms,
					  bool issynthetic )
  : uiSliceSel(p,tp,ZDomain::SI())
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
    stepfld_->box()->setInterval( 1, mCast(int,cs_.hrg.totalNr()) );


    uiSeparator* sep2 = new uiSeparator( this, "nr viewer/table sep" );
    sep2->attach( stretchedBelow, z0fld_ );
    
    posseltbl_ =
	new uiTable( this, uiTable::Setup(1,gathernms_.size()),"Select");
    posseltbl_->attach( ensureBelow, sep2 );
    updatePosTable();

    CallBack cb( mCB(this,uiGatherPosSliceSel,posChged) );
    stepfld_->box()->valueChanging.notify( cb );
    if ( inl0fld_ ) inl0fld_->box()->valueChanging.notify( cb );
    if ( crl0fld_ ) crl0fld_->box()->valueChanging.notify( cb );
    if ( inl1fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );
}


void uiGatherPosSliceSel::updatePosTable()
{
    if ( !posseltbl_ ) return;

    posseltbl_->setColumnLabels( gathernms_ );
    StepInterval<int> trcrg = is2d_ || isinl_ ? cs_.hrg.crlRange()
					      : cs_.hrg.inlRange();
    trcrg.step = stepfld_->box()->getValue();
    const int nrrows = trcrg.nrSteps()+1;
    posseltbl_->setNrRows( nrrows );

    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	BufferString lbl( issynthetic_ ? "Model" : is2d_ ? "Trace"
							 : isinl_ ? "CrossLine"
							 	  : "Inline" );
	lbl += " Nr "; lbl += issynthetic_ ? rowidx : trcrg.atIndex( rowidx );
	posseltbl_->setRowLabel( rowidx, lbl );
	for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
	{
	    RowCol rc( rowidx, colidx );
	    posseltbl_->clearCell( rc );
	    posseltbl_->setCellObject( rc, new uiCheckBox(0,"Select") ); 
	}
    }
}


void uiGatherPosSliceSel::setSelGatherInfos(
	const TypeSet<GatherInfo>& gatherinfos )
{
    gathernms_.erase();
    CubeSampling cs( false );

    StepInterval<int> trcrg( mUdf(int), -mUdf(int), 1 );
    TypeSet<BinID> uniquebids;
    for ( int gidx=0; gidx<gatherinfos.size(); gidx++ )
    {
	GatherInfo ginfo = gatherinfos[gidx];
	uniquebids.addIfNew( ginfo.bid_ );
	trcrg.include( isinl_ ? ginfo.bid_.crl :  ginfo.bid_.inl, false );
	gathernms_.addIfNew( ginfo.gathernm_ );
    }

    const int rgstep = uniquebids.size()==1 ? 1 : isinl_
			    ? abs(uniquebids[1].crl-uniquebids[0].crl)
			    : abs(uniquebids[1].inl-uniquebids[0].inl);
    stepfld_->box()->setValue( rgstep );
    trcrg.step = stepfld_->box()->getValue();
    if ( is2d_ || isinl_ )
	cs_.hrg.setCrlRange( trcrg );
    else
	 cs_.hrg.setInlRange( trcrg );

    setCubeSampling( cs_ );
    updatePosTable();
   
    for ( int rowidx=0; rowidx<trcrg.nrSteps(); rowidx++ )
    {
	const int trcnr = trcrg.atIndex( rowidx );
	BinID bid( isinl_ ? cs_.hrg.start.inl : trcnr,
		   isinl_ ? trcnr : cs_.hrg.start.crl );
	for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
	{
	    RowCol rc( rowidx, colidx );
	    BufferString gathernm = gathernms_.get( colidx );
	    uiObject* obj = posseltbl_->getCellObject( rc );
	    mDynamicCastGet(uiCheckBox*,cb,obj);
	    if ( !cb ) continue;
	    for ( int gidx=0; gidx<gatherinfos.size(); gidx++ )
	    {
		GatherInfo info = gatherinfos[gidx];
		if ( info.bid_ == bid && info.gathernm_== gathernm &&
		     info.isselected_ )
		    cb->setChecked( true );
	    }
	}
    }

}


void uiGatherPosSliceSel::getSelGatherInfos( TypeSet<GatherInfo>& gatherinfos )
{
    gatherinfos.erase();
    const int nrrows = posseltbl_->nrRows();
    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	StepInterval<int> trcrg = is2d_ || isinl_ ? cs_.hrg.crlRange()
	    					  : cs_.hrg.inlRange();
	trcrg.step = stepfld_->box()->getValue();

	const int trcnr = trcrg.atIndex( rowidx );
	BinID bid( isinl_ ? cs_.hrg.start.inl : trcnr,
		   isinl_ ? trcnr : cs_.hrg.start.crl );
	for ( int colidx=0; colidx<gathernms_.size(); colidx++ )
	{
	    RowCol rc( rowidx, colidx );
	    uiObject* uiobj = posseltbl_->getCellObject( rc );
	    if ( !uiobj ) continue;
	    mDynamicCastGet(uiCheckBox*,cb,uiobj);
	    if ( !cb ) continue;

	    GatherInfo ginfo;
	    ginfo.bid_ = bid;
	    ginfo.gathernm_ = gathernms_.get( colidx );
	    ginfo.isselected_ = cb->isChecked();
	    gatherinfos += ginfo;
	}
    }
}


int uiGatherPosSliceSel::step() const
{ return stepfld_->box()->getValue(); }


void uiGatherPosSliceSel::setStep( int ns )
{ stepfld_->box()->setValue(ns); }


void uiGatherPosSliceSel::enableZDisplay( bool yn )
{ z0fld_->display( yn ); z1fld_->display( yn ); }


void uiGatherPosSliceSel::posChged( CallBacker* cb )
{ 
    acceptOK();
    updatePosTable();
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
