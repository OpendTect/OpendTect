/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dposdlg.cc,v 1.2 2011-02-02 09:54:23 cvsbruno Exp $";

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
				0,mTODOHelpID).modal(false))
    , okpushed_(this)					       
{
    uiSliceSel::Type tp = cs.defaultDir() == CubeSampling::Inl ? 
					uiSliceSel::Inl : uiSliceSel::Crl;
    setCtrlStyle( DoAndStay );
    
    sliceselfld_ = new uiGatherPosSliceSel( this, tp );
    sliceselfld_->enableScrollButton( false );
    setCubeSampling( cs );
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
    stepfld_->box()->setInterval( 1, cs_.hrg.totalNr() );

    uiSeparator* sep = new uiSeparator( this, "pos/nr viewer sep" );
    sep->attach( stretchedBelow, z0fld_ );
    nrviewersfld_ = new uiLabeledSpinBox( this, "Number of gathers per line" );
    nrviewersfld_->attach( ensureBelow, sep );
    nrviewersfld_->attach( alignedBelow, z0fld_ );

    CallBack cb( mCB(this,uiGatherPosSliceSel,parsChged) );
    nrviewersfld_->box()->valueChanging.notify( cb );
    stepfld_->box()->valueChanging.notify( cb );
    inl0fld_->box()->valueChanging.notify( cb );
    crl0fld_->box()->valueChanging.notify( cb );
    if ( inl1fld_ ) inl1fld_->valueChanging.notify( cb );
    if ( crl1fld_ ) crl1fld_->valueChanging.notify( cb );
    nrviewersfld_->box()->setInterval( 1, 50 );
    nrviewersfld_->box()->setValue( 16 );

    dynamicrgbox_ = new uiCheckBox( this, msg );
    dynamicrgbox_->attach( rightOf, nrviewersfld_ );
    dynamicrgbox_->activated.notify( 
	    		mCB(this,uiGatherPosSliceSel,dynamicRangeChged) );
    enableDynamicRange( false );
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
