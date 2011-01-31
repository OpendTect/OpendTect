/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dposdlg.cc,v 1.1 2011-01-31 13:03:50 cvsbruno Exp $";

#include "uipsviewer2dposdlg.h"

#include "uibutton.h"
#include "uiseissubsel.h"
#include "uislicesel.h"
#include "uiseparator.h"
#include "uispinbox.h"

#include "seisioobjinfo.h"
#include "zdomain.h"


namespace PreStackView
{


uiViewer2DPosDlg::uiViewer2DPosDlg( uiParent* p, const CubeSampling& cs )
    : uiDialog(p,uiDialog::Setup("Pre-stack Gather display positions",
				0,mTODOHelpID))
{
    setCtrlStyle( DoAndStay );
    setOkText( "&Apply" );
    uiSliceSel::Type tp = cs.defaultDir() == CubeSampling::Inl ? 
					uiSliceSel::Inl : uiSliceSel::Crl;
    sliceselfld_ = new uiGatherPosSliceSel( this, tp );
    sliceselfld_->enableScrollButton( false );
}


uiGatherPosSliceSel::uiGatherPosSliceSel( uiParent* p, uiSliceSel::Type tp )
  : uiSliceSel(p,tp,ZDomain::SI())
{
    setStretch( 2, 2 );
    BufferString msg( "Dynamic " ); 
    msg += isinl_ ? is2d_ ? "Trace" : "Xline" : "Inline"; 
    msg += is2d_ ? " Number" : " Range";

    stepfld_ = new uiLabeledSpinBox( this, "step" );
    stepfld_->attach( rightTo, isinl_ || is2d_ ? crl1fld_ : inl1fld_ ); 

    uiSeparator* sep = new uiSeparator( this, "pos/nr viewer sep" );
    sep->attach( stretchedBelow, z0fld_ );
    nrviewersfld_ = new uiLabeledSpinBox( this, "Number of viewers" );
    nrviewersfld_->attach( ensureBelow, sep );
    nrviewersfld_->attach( alignedBelow, z0fld_ );

    CallBack cb( mCB(this,uiGatherPosSliceSel,parsChged) );
    nrviewersfld_->box()->valueChanging.notify( cb );
    stepfld_->box()->valueChanging.notify( cb );
    inl0fld_->box()->valueChanging.notify( cb );
    crl0fld_->box()->valueChanging.notify( cb );
    if ( inl1fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );

    dynamicrgbox_ = new uiCheckBox( this, msg );
    dynamicrgbox_->attach( rightOf, nrviewersfld_ );
    dynamicrgbox_->activated.notify( 
	    		mCB(this,uiGatherPosSliceSel,dynamicRangeChged) );
    enableDynamicRange( false );
}


const CubeSampling& uiGatherPosSliceSel::cubeSampling() 
{
    acceptOK();
    cs_ = getCubeSampling();
    cs_.hrg.step = BinID( step(), step() );
    return cs_;
}


int uiGatherPosSliceSel::step() const
{ return stepfld_->box()->getValue(); }


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
    inl0fld_->setSensitive( !isdynam );
    crl0fld_->setSensitive( !isdynam );
    stepfld_->setSensitive( !isdynam );
    if ( inl1fld_ ) inl1fld_->setSensitive( !isdynam );
    if ( crl1fld_ ) crl1fld_->setSensitive( !isdynam );
}


void uiGatherPosSliceSel::enableZDisplay( bool yn )
{ z0fld_->display( yn ); z1fld_->display( yn ); }


void uiGatherPosSliceSel::parsChged( CallBacker* cb )
{ 
    acceptOK();
    mDynamicCastGet(uiSpinBox*,box,cb)
    if ( !box ) return;
    const bool isvwrbox = box == nrviewersfld_->box();
    uiSpinBox* spbox = isvwrbox ? box : stepfld_->box(); 
    uiSpinBox* revbox = isvwrbox ? stepfld_->box() : nrviewersfld_->box();
    const int divnr = spbox->getValue();
    if ( divnr > 0 ) 
	revbox->setValue( (int)(cs_.hrg.totalNr()/divnr) );
}

};
