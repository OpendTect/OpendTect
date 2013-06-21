/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiraytrace1d.h"
#include "uisplitter.h"
#include "uiseiswvltsel.h"
#include "uisynthgendlg.h"

#include "synthseis.h"
#include "stratsynth.h"


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if ( s ) uiMSG().error(s); act; }

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp) 
    : uiDialog(p,uiDialog::Setup("Specify Synthetic Parameters",mNoDlgTitle,
				 "103.4.4").modal(false))
    , stratsynth_(gp)
    , genNewReq(this)
    , synthRemoved(this)
    , synthChanged(this)
{
    setOkText( "Apply" );
    uiGroup* syntlistgrp = new uiGroup( this, "Synthetics List" );
    uiLabeledListBox* llb =
	new uiLabeledListBox( syntlistgrp, "Synthetics", false,
			      uiLabeledListBox::AboveMid );
    synthnmlb_ = llb->box();
    synthnmlb_->selectionChanged.notify(
	    mCB(this,uiSynthGenDlg,changeSyntheticsCB) );
    uiPushButton* rembut =
	new uiPushButton( syntlistgrp, "Remove selected",
			  mCB(this,uiSynthGenDlg,removeSyntheticsCB), true );
    rembut->attach( leftAlignedBelow, llb );

    uiGroup* pargrp = new uiGroup( this, "Parameter Group" );
    pargrp->setStretch( 1, 1 );
    BufferStringSet types( SynthGenParams::SynthTypeNames() );
    uiLabeledComboBox* lblcbx =
	new uiLabeledComboBox( pargrp, types, "Synthethic type" );
    typefld_ = lblcbx->box();
    typefld_->selectionChanged.notify( mCB(this,uiSynthGenDlg,typeChg) );

    psselfld_ = new uiLabeledComboBox( pargrp, "Input PreStack" );
    psselfld_->attach( alignedBelow, lblcbx );

    FloatInpIntervalSpec finpspec(false);
    finpspec.setLimits( Interval<float>(0,90) );
    finpspec.setDefaultValue( Interval<float>(0,30) );
    angleinpfld_ = new uiGenInput( pargrp, "Angle Range", finpspec );
    angleinpfld_->attach( alignedBelow, psselfld_ );

    uiRayTracer1D::Setup rsu; rsu.dooffsets(true).convertedwaves(true);
    rtsel_ = new uiRayTracerSel( pargrp, rsu );
    rtsel_->usePar( stratsynth_.genParams().raypars_ ); 
    rtsel_->attach( alignedBelow, angleinpfld_ );
    rtsel_->offsetChanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );

    wvltfld_ = new uiSeisWaveletSel( pargrp );
    wvltfld_->newSelection.notify( mCB(this,uiSynthGenDlg,parsChanged) );
    wvltfld_->attach( alignedBelow, rtsel_ );
    wvltfld_->setFrame( false );
    
    nmofld_ = new uiGenInput( pargrp, "Apply NMO corrections",
			      BoolInpSpec(true) );
    mAttachCB( nmofld_->valuechanged, uiSynthGenDlg, parsChanged);
    nmofld_->attach( alignedBelow, wvltfld_ );
    
    FloatInpSpec inpspec;
    inpspec.setLimits( Interval<float>(1,500) );
    stretchmutelimitfld_ = new uiGenInput(pargrp, "Stretch mute (%)", inpspec );
    stretchmutelimitfld_->attach( alignedBelow, nmofld_ );
    
    mutelenfld_ = new uiGenInput( pargrp, "Mute taper-length (ms)",
				      FloatInpSpec() );
    mutelenfld_->attach( alignedBelow, stretchmutelimitfld_ );

    namefld_ = new uiGenInput( pargrp, "Name" );
    namefld_ ->attach( alignedBelow, mutelenfld_ );
    namefld_->valuechanged.notify( mCB(this,uiSynthGenDlg,nameChanged) );

    gennewbut_ = new uiPushButton( pargrp, "&Add as new", true );
    gennewbut_->activated.notify( mCB(this,uiSynthGenDlg,genNewCB) );
    gennewbut_->attach( alignedBelow, namefld_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( syntlistgrp );
    splitter->addGroup( pargrp );

    updateSynthNames();
    synthnmlb_->setSelected( 0, true );
    changeSyntheticsCB( 0 );
}


void uiSynthGenDlg::getPSNames( BufferStringSet& synthnms ) 
{
    synthnms.erase();

    for ( int synthidx=0; synthidx<stratsynth_.nrSynthetics(); synthidx++ )
    {
	SynthGenParams genparams;
	SyntheticData* synth = stratsynth_.getSyntheticByIdx( synthidx );
	if ( !synth ) continue;
	synth->fillGenParams( genparams );
	if ( !genparams.isPreStack() ) continue;
	bool donmo = false;
	genparams.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(),donmo );
	if ( !donmo ) continue;
	synthnms.add( genparams.name_ );
    }
}


void uiSynthGenDlg::updateSynthNames() 
{
    synthnmlb_->setEmpty();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	synthnmlb_->addItem( sd->name() );
    }
}


void uiSynthGenDlg::changeSyntheticsCB( CallBacker* )
{
    FixedString synthnm( synthnmlb_->textOfItem(synthnmlb_->nextSelected(-1)) );
    if ( synthnm.isEmpty() ) return;
    SyntheticData* sd = stratsynth_.getSynthetic( synthnm );
    if ( !sd ) return;
    sd->fillGenParams( stratsynth_.genParams() );
    putToScreen();
}


void uiSynthGenDlg::nameChanged( CallBacker* )
{
    stratsynth_.genParams().name_ = namefld_->text();
}


void uiSynthGenDlg::parsChanged( CallBacker* )
{
    if ( !getFromScreen() ) return;
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::removeSyntheticsCB( CallBacker* )
{
    if ( synthnmlb_->size()==1 )
	return uiMSG().error( "Cannot remove all synthetics" );

    SynthGenParams cursgp = stratsynth_.genParams();
    int nrofzerooffs = 0;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;
	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( sgp.synthtype_ == SynthGenParams::ZeroOffset )
	{
	    nrofzerooffs++;
	    continue;
	}
	if ( cursgp.isPreStack() &&
	     (sgp.synthtype_ == SynthGenParams::AngleStack ||
	      sgp.synthtype_ == SynthGenParams::AVOGradient) &&
	     sgp.inpsynthnm_ == cursgp.name_ )
	{
	    BufferString msg( sgp.name_.buf(), "will also be removed as "
					       "it is dependent on ",
			      cursgp.name_.buf() );
	    msg += "Do you want to remove the synthetics?";
	    if ( !uiMSG().askGoOn(msg) )
		return;
	    BufferString synthname( sgp.name_ );
	    synthnmlb_->removeItem( synthnmlb_->currentItem() );
	    synthRemoved.trigger( synthname );
	    break;
	}
    }

    if ( cursgp.synthtype_ == SynthGenParams::ZeroOffset && nrofzerooffs<=1 )
    {
	BufferString msg( "Cannot remove ", cursgp.name_.buf(),
			  " as there should be atleast one 0 offset synthetic");
	return uiMSG().error( msg );
    }

    BufferString synthname( synthnmlb_->getText() );
    synthnmlb_->removeItem( synthnmlb_->currentItem() );
    synthRemoved.trigger( synthname );
}


void uiSynthGenDlg::updateFieldSensitivity()
{
    SynthGenParams::SynthType synthtype =
	(SynthGenParams::SynthType)typefld_->currentItem();
    const bool isps = synthtype==SynthGenParams::PreStack;
    const bool psbased = synthtype == SynthGenParams::AngleStack ||
			 synthtype == SynthGenParams::AVOGradient;
    nmofld_->display( isps );
    rtsel_->display( isps );
    rtsel_->current()->displayOffsetFlds( isps || psbased );
    rtsel_->current()->setOffsetRange( uiRayTracer1D::Setup().offsetrg_ );
    psselfld_->display( psbased );
    wvltfld_->display( !psbased );
    mutelenfld_->display( isps );
    stretchmutelimitfld_->display( isps );
    angleinpfld_->display( psbased );
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    updateFieldSensitivity();
    stratsynth_.genParams().synthtype_ =
	(SynthGenParams::SynthType)typefld_->currentItem();
    stratsynth_.genParams().raypars_.setEmpty();
    if ( typefld_->currentItem()==1 )
	rtsel_->setCurrent( 0 );
    rtsel_->fillPar( stratsynth_.genParams().raypars_ );
    putToScreen();
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::putToScreen()
{
    const SynthGenParams& genparams = stratsynth_.genParams();
    wvltfld_->setInput( genparams.wvltnm_ );
    namefld_->setText( genparams.name_ );
    
    const bool isps = genparams.isPreStack();
    typefld_->setCurrentItem( (int)genparams.synthtype_ );
    if ( genparams.synthtype_ == SynthGenParams::ZeroOffset )
	rtsel_->setCurrent( 0 );

    if ( genparams.synthtype_ == SynthGenParams::AngleStack ||
	 genparams.synthtype_ == SynthGenParams::AVOGradient )
    {
	BufferStringSet psnms;
	getPSNames( psnms );
	psselfld_->box()->setEmpty();
	psselfld_->box()->addItems( psnms );
	psselfld_->box()->setCurrentItem( genparams.inpsynthnm_ );
	angleinpfld_->setValue( genparams.anglerg_ ); 
	updateFieldSensitivity();
	return;
    }

    TypeSet<float> offsets;
    genparams.raypars_.get( RayTracer1D::sKeyOffset(), offsets );
        
    bool donmo = false;
    genparams.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(), donmo );
    nmofld_->setValue( donmo );
    
    if ( isps )
    {
	float mutelen = Seis::SynthGenBase::cStdMuteLength();
	genparams.raypars_.get(
		Seis::SynthGenBase::sKeyMuteLength(), mutelen );
	mutelenfld_->setValue(
	    mIsUdf(mutelen) ? mutelen : mutelen *ZDomain::Time().userFactor());
	
	float stretchlimit = Seis::SynthGenBase::cStdStretchLimit();
	genparams.raypars_.get(
			Seis::SynthGenBase::sKeyStretchLimit(), stretchlimit );
	stretchmutelimitfld_->setValue( mToPercent( stretchlimit ) );
    }

    rtsel_->usePar( genparams.raypars_ );
    updateFieldSensitivity();
}


bool uiSynthGenDlg::getFromScreen() 
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);
    
    if ( mutelenfld_->attachObj()->isDisplayed() &&
	 (mIsUdf(mutelenfld_->getfValue() ) || mutelenfld_->getfValue()<0) )
	mErrRet( "The mutelength must be more than zero.", return false );
    
    if ( stretchmutelimitfld_->attachObj()->isDisplayed() &&
	(mIsUdf(stretchmutelimitfld_->getfValue()) ||
	 stretchmutelimitfld_->getfValue()<0) )
	mErrRet( "The stretch mute must be more than 0%", return false );
    
    stratsynth_.genParams().raypars_.setEmpty();
    
    SynthGenParams& genparams = stratsynth_.genParams();
    genparams.synthtype_ = (SynthGenParams::SynthType)typefld_->currentItem(); 
    const bool isps = !typefld_->currentItem();

    if ( genparams.synthtype_ == SynthGenParams::AngleStack ||
	 genparams.synthtype_ == SynthGenParams::AVOGradient )
    {
	SynthGenParams::SynthType synthtype = genparams.synthtype_;
	if ( psselfld_->box()->isEmpty() )
	    mErrRet( "Cannot generate an angle stack synthetics without any "
		     "NMO corrected PreStack.", return false );
	SyntheticData* inppssd = stratsynth_.getSynthetic(
		psselfld_->box()->textOfItem(psselfld_->box()->currentItem()) );
	if ( !inppssd )
	    mErrRet("Problem with Input Pre-Stack synthetic data",return false);
	
	inppssd->fillGenParams( genparams );
	genparams.name_ = nm;
	genparams.synthtype_ = synthtype;
	genparams.inpsynthnm_ = inppssd->name();
	genparams.anglerg_ = angleinpfld_->getFInterval();
	return true;
    }

    rtsel_->fillPar( genparams.raypars_ );

    if ( !isps )
	RayTracer1D::setIOParsToZeroOffset( genparams.raypars_ );

    genparams.wvltnm_ = wvltfld_->getName();
    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    bool donmo = isps ? nmofld_->getBoolValue() : false;
    genparams.raypars_.setYN( Seis::SynthGenBase::sKeyNMO(), donmo );
    genparams.name_ = namefld_->text();
    
    if ( isps )
    {
	genparams.raypars_.set( Seis::SynthGenBase::sKeyMuteLength(),
		     mutelenfld_->getfValue() / ZDomain::Time().userFactor() );
	genparams.raypars_.set(
		Seis::SynthGenBase::sKeyStretchLimit(),
		mFromPercent( stretchmutelimitfld_->getfValue()) );
    }

    return true;
}


void uiSynthGenDlg::updateWaveletName() 
{
    wvltfld_->setInput( stratsynth_.genParams().wvltnm_ );
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


bool uiSynthGenDlg::acceptOK( CallBacker* )
{
    if ( !synthnmlb_->nrSelected() ) return true;

    if ( !getFromScreen() ) return false;
    BufferString synthname( synthnmlb_->getText() );
    synthChanged.trigger( synthname );
    
    return true;
}


bool uiSynthGenDlg::isCurSynthChanged() const
{
    const int selidx = synthnmlb_->nextSelected(-1);
    if ( selidx < 0 ) return false;
    BufferString selstr = synthnmlb_->textOfItem( selidx );
    SyntheticData* sd = stratsynth_.getSynthetic( selstr );
    if ( !sd ) return true;
    SynthGenParams genparams;
    sd->fillGenParams( genparams );
    return !(genparams == stratsynth_.genParams());
}


bool uiSynthGenDlg::rejectOK( CallBacker* )
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);

    if ( !getFromScreen() ) return false;
    if ( !isCurSynthChanged() )
	return true;
    BufferString msg( "Selected synthetic has been changed. "
	    	      "Do you want to Apply the changes?" );
    if ( uiMSG().askGoOn(msg,"Apply","Do not Apply") )
	acceptOK( 0 );

    return true;
}


bool uiSynthGenDlg::genNewCB( CallBacker* )
{
    if ( !getFromScreen() ) return false;

    if ( synthnmlb_->isPresent(stratsynth_.genParams().name_) )
    {
	BufferString msg( "Synthectic data of name '" );
	msg += stratsynth_.genParams().name_;
	msg += "' is already present. Please choose a different name";
	uiMSG().error( msg );
	return false;
    }

    genNewReq.trigger();
    return true;
}

