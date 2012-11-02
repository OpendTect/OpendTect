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
#include "uitoolbutton.h"

#include "seisioobjinfo.h"
#include "zdomain.h"


#define cStartNrVwrs 8

namespace PreStackView
{

uiViewer2DPosDlg::uiViewer2DPosDlg( uiParent* p, bool is2d, 
					const CubeSampling& cs )
    : uiDialog(p,uiDialog::Setup("Pre-stack Gather display positions",
				0,"50.2.3").modal(false))
    , okpushed_(this)
    , is2d_(is2d)		     
{
    uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD :
	cs.defaultDir()==CubeSampling::Inl ? uiSliceSel::Inl : uiSliceSel::Crl;
    setCtrlStyle( DoAndStay );
    
    sliceselfld_ = new uiGatherPosSliceSel( this, tp );
    sliceselfld_->enableScrollButton( false );
    if ( is2d_ )
	sliceselfld_->setMaxCubeSampling( cs );
    setCubeSampling( cs );
    setOkText( "&Apply" );
    setNrViewers( cStartNrVwrs );
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
    const int nrvwrs = nrViewers();
    sliceselfld_->setStep( mMAX( cs.hrg.step.crl, cs.hrg.step.inl ) );
    sliceselfld_->setCubeSampling(cs);
    setNrViewers( nrvwrs );
}


uiGatherPosSliceSel::uiGatherPosSliceSel( uiParent* p, uiSliceSel::Type tp )
  : uiSliceSel(p,tp,ZDomain::SI())
{
    setStretch( 2, 2 );
    BufferString msg( "Dynamic " ); 
    const char* linetxt = isinl_ ? is2d_ ? "Trace" : "Xline" : "Inline";
    msg += linetxt;
    msg += is2d_ ? " Number" : " Range";

    stepfld_ = new uiLabeledSpinBox( this, "step" );
    stepfld_->attach( rightTo, isinl_ || is2d_ ? crl1fld_ : inl1fld_ ); 
    stepfld_->box()->setInterval( 1, mCast(int,cs_.hrg.totalNr()) );

    uiSeparator* sep = new uiSeparator( this, "pos/nr viewer sep" );
    sep->attach( stretchedBelow, z0fld_ );
    nrviewersfld_ = new uiLabeledSpinBox( this, "Number of gathers per line" );
    nrviewersfld_->attach( ensureBelow, sep );
    nrviewersfld_->attach( alignedBelow, z0fld_ );
    nrviewersfld_->box()->valueChanging.notify( 
	    mCB(this,uiGatherPosSliceSel,nrViewersChged) );
    nrviewersfld_->box()->setInterval( 1, 100 );

    dynamicrgbox_ = new uiCheckBox( this, msg );
    dynamicrgbox_->attach( rightOf, nrviewersfld_ );
    dynamicrgbox_->activated.notify( 
	    		mCB(this,uiGatherPosSliceSel,dynamicRangeChged) );
    enableDynamicRange( false );

    CallBack cb( mCB(this,uiGatherPosSliceSel,posChged) );
    stepfld_->box()->valueChanging.notify( cb );
    if ( inl0fld_ ) inl0fld_->box()->valueChanging.notify( cb );
    if ( crl0fld_ ) crl0fld_->box()->valueChanging.notify( cb );
    if ( inl0fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );
}


int uiGatherPosSliceSel::step() const
{ return stepfld_->box()->getValue(); }


void uiGatherPosSliceSel::setStep( int ns )
{ stepfld_->box()->setValue(ns); }


int uiGatherPosSliceSel::nrViewers() const
{ return nrviewersfld_->box()->getValue(); }


void uiGatherPosSliceSel::setNrViewers( int nrvwrs )
{ nrviewersfld_->box()->setValue( nrvwrs ); }


void uiGatherPosSliceSel::enableDynamicRange( bool yn )
{ 
    dynamicrgbox_->display( yn ); 
    dynamicrgbox_->setChecked( yn );
}


void uiGatherPosSliceSel::dynamicRangeChged( CallBacker* )
{
    const bool isdynam = dynamicrgbox_->isChecked();
    if ( inl0fld_ ) inl0fld_->setSensitive( !isdynam );
    if ( crl0fld_ ) crl0fld_->setSensitive( !isdynam );
    stepfld_->setSensitive( !isdynam );
    if ( inl0fld_ ) inl1fld_->setSensitive( !isdynam );
    if ( crl1fld_ ) crl1fld_->setSensitive( !isdynam );
}


void uiGatherPosSliceSel::enableZDisplay( bool yn )
{ z0fld_->display( yn ); z1fld_->display( yn ); }


void uiGatherPosSliceSel::posChged( CallBacker* cb )
{ 
    acceptOK();
    const int divnr = stepfld_->box()->getValue(); 
    if ( divnr > 0 ) 
    {
	NotifyStopper ns( nrviewersfld_->box()->valueChanging );
	Interval<int> rg = isinl_ || is2d_ ? cs_.hrg.crlRange() 
			        	   : cs_.hrg.inlRange();
	nrviewersfld_->box()->setValue( rg.width()/divnr +1 );
    }
}


void uiGatherPosSliceSel::nrViewersChged( CallBacker* cb )
{ 
    acceptOK();
    const int divnr = nrviewersfld_->box()->getValue()-1; 
    if ( divnr > 0 ) 
    {
	NotifyStopper ns( stepfld_->box()->valueChanging );
	Interval<int> rg = isinl_ || is2d_ ? cs_.hrg.crlRange() 
					   : cs_.hrg.inlRange();
	stepfld_->box()->setValue( rg.width()/divnr );
    }
}


bool uiGatherPosSliceSel::isDynamicRange() const
{ return dynamicrgbox_->isChecked(); }


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



