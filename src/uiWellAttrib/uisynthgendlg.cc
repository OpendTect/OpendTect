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
#include "uisynthseis.h"
#include "uisynthgendlg.h"

#include "synthseis.h"
#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "hiddenparam.h"
#include "od_helpids.h"

static HiddenParam<uiSynthGenDlg,uiSynthSeisGrp*> synthseisset( 0 );

#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if ( s ) uiMSG().error(s); act; }

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp)
    : uiDialog(p,uiDialog::Setup("Specify Synthetic Parameters",mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
                                 .modal(false))
    , stratsynth_(gp)
    , genNewReq(this)
    , synthRemoved(this)
    , synthChanged(this)
    , wvltfld_(0)
    , rtsel_(0)
    , uisynthcorrgrp_(0)
{
    setOkText( "&Apply" );
    setCancelText( "&Dismiss" );
    uiGroup* syntlistgrp = new uiGroup( this, "Synthetics List" );
    uiLabeledListBox* llb =
	new uiLabeledListBox( syntlistgrp, "Synthetics", OD::ChooseOnlyOne,
			      uiLabeledListBox::AboveMid );
    synthnmlb_ = llb->box();
    synthnmlb_->selectionChanged.notify(
	    mCB(this,uiSynthGenDlg,changeSyntheticsCB) );
    uiPushButton* rembut =
	new uiPushButton( syntlistgrp, "Remove selected",
			  mCB(this,uiSynthGenDlg,removeSyntheticsCB), true );
    rembut->attach( leftAlignedBelow, llb );

    uiGroup* rightgrp = new uiGroup( this, "Parameter Group" );
    rightgrp->setStretch( 1, 1 );

    uiGroup* toppargrp = new uiGroup( rightgrp, "Parameter Group - top part" );
    BufferStringSet types( SynthGenParams::SynthTypeNames() );
    const int stratpropidx =
	types.indexOf( SynthGenParams::toString(SynthGenParams::StratProp) );
    types.removeSingle( stratpropidx );
    uiLabeledComboBox* lblcbx =
	new uiLabeledComboBox( toppargrp, types, "Synthetic type" );
    typefld_ = lblcbx->box();
    typefld_->selectionChanged.notify( mCB(this,uiSynthGenDlg,typeChg) );

    psselfld_ = new uiLabeledComboBox( toppargrp, "Input PreStack" );
    psselfld_->attach( alignedBelow, lblcbx );

    FloatInpIntervalSpec finpspec(false);
    finpspec.setLimits( Interval<float>(0,90) );
    finpspec.setDefaultValue( Interval<float>(0,30) );
    angleinpfld_ = new uiGenInput( toppargrp, "Angle Range", finpspec );
    angleinpfld_->attach( alignedBelow, psselfld_ );
    angleinpfld_->valuechanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );

    uiRayTracer1D::Setup rsu;
    rsu.dooffsets(true).convertedwaves(true);
    rsu.showzerooffsetfld(false);
    uiSynthSeisGrp* synthseis = new uiSynthSeisGrp( toppargrp, rsu );
    synthseis->parsChanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );
    synthseis->attach( alignedBelow, angleinpfld_ );
    toppargrp->setHAlignObj( synthseis );
    synthseisset.setParam( this, synthseis );

    uiGroup* botpargrp = new uiGroup( rightgrp, "Parameter Group - Last Part" );
    botpargrp->attach( alignedBelow, toppargrp );

    namefld_ = new uiGenInput( botpargrp, "		  Name" );
    namefld_->valuechanged.notify( mCB(this,uiSynthGenDlg,nameChanged) );
    botpargrp->setHAlignObj( namefld_ );

    gennewbut_ = new uiPushButton( botpargrp, "&Add as new", true );
    gennewbut_->activated.notify( mCB(this,uiSynthGenDlg,genNewCB) );
    gennewbut_->attach( alignedBelow, namefld_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( syntlistgrp );
    splitter->addGroup( rightgrp );

    postFinalise().notify( mCB(this,uiSynthGenDlg,finaliseDone) );
}


uiSynthGenDlg::~uiSynthGenDlg()
{
    synthseisset.removeParam( this );
}


void uiSynthGenDlg::finaliseDone( CallBacker* )
{
    updateSynthNames();
    synthnmlb_->setCurrentItem( 0 );
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

	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	synthnmlb_->addItem( sd->name() );
    }
}


void uiSynthGenDlg::changeSyntheticsCB( CallBacker* )
{
    FixedString synthnm( synthnmlb_->getText() );
    if ( synthnm.isEmpty() ) return;
    SyntheticData* sd = stratsynth_.getSynthetic( synthnm.buf() );
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
	{ uiMSG().error( "Cannot remove all synthetics" ); return; }
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return uiMSG().error( "No synthetic selected" );

    int nrofzerooffs = 0;
    BufferStringSet synthstoberemoved;
    SynthGenParams sgptorem = stratsynth_.genParams();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd || sd->isPS() ) continue;
	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( sgp.synthtype_ == SynthGenParams::ZeroOffset )
	{
	    nrofzerooffs++;
	    continue;
	}

	if ( sgptorem.isPreStack() &&
	     (sgp.synthtype_ == SynthGenParams::AngleStack ||
	      sgp.synthtype_ == SynthGenParams::AVOGradient) &&
	      sgp.inpsynthnm_ == sgptorem.name_ )
	    synthstoberemoved.add( sgp.name_ );
    }

    if ( sgptorem.synthtype_ == SynthGenParams::ZeroOffset && nrofzerooffs<=1 )
    {
	BufferString msg( "Cannot remove ", sgptorem.name_.buf(),
			  " as there should be atleast one 0 offset synthetic");
	return uiMSG().error( msg );
    }

    if ( synthstoberemoved.isEmpty() )
    {
	synthnmlb_->removeItem( synthnmlb_->indexOf(sgptorem.name_) );
	synthRemoved.trigger( sgptorem.name_ );
	return;
    }

    BufferString msg( synthstoberemoved.getDispString(),
		      " will also be removed as it is dependent on '",
		      sgptorem.name_.buf() );
    msg += "'. Do you want to remove the synthetics?";
    synthstoberemoved.add( sgptorem.name_ );
    if ( !uiMSG().askGoOn(msg) )
	return;

    for ( int idx=0; idx<synthstoberemoved.size(); idx++ )
    {
	const BufferString& synthnm = synthstoberemoved.get( idx );
	synthnmlb_->removeItem( synthnmlb_->indexOf(synthnm) );
	synthRemoved.trigger( synthnm );
    }
}


void uiSynthGenDlg::updateFieldSensitivity()
{
    SynthGenParams::SynthType synthtype =
	SynthGenParams::parseEnumSynthType( typefld_->text() );
    const bool psbased = synthtype == SynthGenParams::AngleStack ||
			 synthtype == SynthGenParams::AVOGradient;
    synthseisset.getParam(this)->updateFieldDisplay();
    psselfld_->display( psbased );
    if ( psbased )
	synthseisset.getParam(this)->updateDisplayForPSBased();
    angleinpfld_->display( psbased );
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    updateFieldSensitivity();
    stratsynth_.genParams().synthtype_ =
	SynthGenParams::parseEnumSynthType( typefld_->text() );
    stratsynth_.genParams().setDefaultValues();
    putToScreen();
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::putToScreen()
{
    NotifyStopper parschgstopper( synthseisset.getParam(this)->parsChanged );
    NotifyStopper angparschgstopper( angleinpfld_->valuechanged );
    const SynthGenParams& genparams = stratsynth_.genParams();
    synthseisset.getParam(this)->setWavelet( genparams.wvltnm_ );
    namefld_->setText( genparams.name_ );

    typefld_->setCurrentItem( SynthGenParams::toString(genparams.synthtype_) );
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

    synthseisset.getParam(this)->usePar( genparams.raypars_ );
    updateFieldSensitivity();
}


bool uiSynthGenDlg::getFromScreen()
{
    const char* nm = namefld_->text();
    if ( !nm )
	mErrRet("Please specify a valid name",return false);

    stratsynth_.genParams().raypars_.setEmpty();

    SynthGenParams& genparams = stratsynth_.genParams();
    genparams.synthtype_ = SynthGenParams::parseEnumSynthType(typefld_->text());

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
	    mErrRet("Problem with Input Prestack synthetic data",return false);

	inppssd->fillGenParams( genparams );
	genparams.name_ = nm;
	genparams.synthtype_ = synthtype;
	genparams.inpsynthnm_ = inppssd->name();
	genparams.anglerg_ = angleinpfld_->getFInterval();
	return true;
    }

    synthseisset.getParam(this)->fillPar( genparams.raypars_ );
    genparams.wvltnm_ = synthseisset.getParam(this)->getWaveletName();
    genparams.name_ = namefld_->text();

    return true;
}


void uiSynthGenDlg::updateWaveletName()
{
    synthseisset.getParam(this)->setWavelet( stratsynth_.genParams().wvltnm_ );
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


bool uiSynthGenDlg::acceptOK( CallBacker* )
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return true;

    if ( !getFromScreen() )
	return false;

    BufferString synthname( synthnmlb_->getText() );
    synthChanged.trigger( synthname );

    return true;
}


bool uiSynthGenDlg::isCurSynthChanged() const
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx < 0 )
	return false;
    BufferString selstr = synthnmlb_->textOfItem( selidx );
    SyntheticData* sd = stratsynth_.getSynthetic( selstr );
    if ( !sd )
	return true;
    SynthGenParams genparams;
    sd->fillGenParams( genparams );
    return !(genparams == stratsynth_.genParams());
}


bool uiSynthGenDlg::rejectOK( CallBacker* )
{
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
