/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uisynthgendlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uisynthseis.h"

#include "synthseis.h"
#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "od_helpids.h"


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if (!s.isEmpty()) uiMSG().error(s); act;}

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp )
    : uiDialog(p,uiDialog::Setup(tr("Specify Synthetic Parameters"),mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
				.modal(false))
    , stratsynth_(gp)
    , genNewReq(this)
    , synthRemoved(this)
    , synthChanged(this)
    , synthDisabled(this)
{
    setForceFinalise( true );
    setOkText( uiStrings::sApply() );
    setCancelText( uiStrings::sClose() );
    uiGroup* syntlistgrp = new uiGroup( this, "Synthetics List" );
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Synthetics"),
			 uiListBox::AboveMid );
    synthnmlb_ = new uiListBox( syntlistgrp, su );
    synthnmlb_->setHSzPol( uiObject::SmallVar );
    synthnmlb_->selectionChanged.notify(
	    mCB(this,uiSynthGenDlg,changeSyntheticsCB) );
    uiPushButton* rembut =
	new uiPushButton( syntlistgrp, tr("Remove selected"),
			  mCB(this,uiSynthGenDlg,removeSyntheticsCB), true );
    rembut->attach( leftAlignedBelow, synthnmlb_ );

    uiGroup* rightgrp = new uiGroup( this, "Parameter Group" );
    rightgrp->setStretch( 1, 1 );

    uiGroup* toppargrp = new uiGroup( rightgrp, "Parameter Group - top part" );
    BufferStringSet types( SynthGenParams::SynthTypeNames() );
    const int stratpropidx =
	types.indexOf( SynthGenParams::toString(SynthGenParams::StratProp) );
    types.removeSingle( stratpropidx );
    uiLabeledComboBox* lblcbx =
	new uiLabeledComboBox( toppargrp, types, tr("Synthetic type") );
    typefld_ = lblcbx->box();
    typefld_->selectionChanged.notify( mCB(this,uiSynthGenDlg,typeChg) );

    psselfld_ = new uiLabeledComboBox( toppargrp, tr("Input Prestack") );
    psselfld_->attach( alignedBelow, lblcbx );

    FloatInpIntervalSpec finpspec(false);
    finpspec.setLimits( Interval<float>(0,90) );
    finpspec.setDefaultValue( Interval<float>(0,30) );
    angleinpfld_ = new uiGenInput( toppargrp, tr("Angle range"), finpspec );
    angleinpfld_->attach( alignedBelow, psselfld_ );
    angleinpfld_->valuechanged.notify( mCB(this,uiSynthGenDlg,angleInpChanged));

    uiRayTracer1D::Setup rsu;
    rsu.dooffsets(true).convertedwaves(true).showzerooffsetfld(false);
    synthseis_ = new uiSynthSeisGrp( toppargrp, rsu );
    synthseis_->parsChanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );
    synthseis_->attach( alignedBelow, lblcbx );
    toppargrp->setHAlignObj( synthseis_ );

    uiGroup* botpargrp = new uiGroup( rightgrp, "Parameter Group - Last Part" );
    botpargrp->attach( alignedBelow, toppargrp );

    namefld_ = new uiGenInput( botpargrp, uiStrings::sName() );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->valuechanged.notify( mCB(this,uiSynthGenDlg,nameChanged) );
    botpargrp->setHAlignObj( namefld_ );

    gennewbut_ = new uiPushButton( botpargrp, tr("Add as new"), true );
    gennewbut_->activated.notify( mCB(this,uiSynthGenDlg,genNewCB) );
    gennewbut_->attach( alignedBelow, namefld_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( syntlistgrp );
    splitter->addGroup( rightgrp );

    postFinalise().notify( mCB(this,uiSynthGenDlg,finaliseDone) );
}


void uiSynthGenDlg::finaliseDone( CallBacker* )
{
    typeChg( nullptr );
    updateSynthNames();
    synthnmlb_->setCurrentItem( 0 );
    changeSyntheticsCB( nullptr );
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
	synthnmlb_->addItem( toUiString(sd->name()) );
    }
}


void uiSynthGenDlg::changeSyntheticsCB( CallBacker* )
{
    FixedString synthnm( synthnmlb_->getText() );
    if ( synthnm.isEmpty() )
	return;

    SyntheticData* sd = stratsynth_.getSynthetic( synthnm.buf() );
    if ( !sd )
	return;

    sd->fillGenParams( stratsynth_.genParams() );
    putToScreen();
}


void uiSynthGenDlg::nameChanged( CallBacker* )
{
    stratsynth_.genParams().name_ = namefld_->text();
}


void uiSynthGenDlg::angleInpChanged( CallBacker* )
{
    NotifyStopper angparschgstopper( angleinpfld_->valuechanged );
    const SynthGenParams& genparams = stratsynth_.genParams();
    if ( genparams.anglerg_ == angleinpfld_->getFInterval() )
	return;

    if ( !getFromScreen() )
    {
	angleinpfld_->setValue( genparams.anglerg_ );
	return;
    }

    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::parsChanged( CallBacker* )
{
    if ( !getFromScreen() ) return;
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


bool uiSynthGenDlg::prepareSyntheticToBeChanged( bool toberemoved )
{
    if ( synthnmlb_->size()==1 && toberemoved )
	mErrRet( tr("Cannot remove all synthetics"), return false );

    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	mErrRet( tr("No synthetic selected"), return false );

    const BufferString synthtochgnm( synthnmlb_->getText() );
    const SyntheticData* sdtochg = stratsynth_.getSynthetic( synthtochgnm );
    if ( !sdtochg )
	mErrRet( tr("Cannot find synthetic data '%1'").arg(synthtochgnm),
		 return false );

    SynthGenParams sdtochgsgp;
    sdtochg->fillGenParams( sdtochgsgp );
    if ( !toberemoved )
    {
	const SynthGenParams& cursgp = stratsynth_.genParams();
	if ( cursgp == sdtochgsgp )
	    return true;
    }

    BufferStringSet synthstobedisabled;
    const SynthGenParams& sgptorem = stratsynth_.genParams();
    int nrofzerooffs = 0;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd || sd->isPS() )
	    continue;

	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( sgp.synthtype_ == SynthGenParams::ZeroOffset )
	{
	    nrofzerooffs++;
	    continue;
	}

	if ( sgptorem.isPreStack() && sgp.isPSBased() &&
	     sgp.inpsynthnm_ == sgptorem.name_ )
	    synthstobedisabled.add( sgp.name_ );
    }

    if ( toberemoved &&
	 sgptorem.synthtype_ == SynthGenParams::ZeroOffset && nrofzerooffs<=1 )
	mErrRet( tr("Cannot remove %1 as there should be "
		    "at least one 0 offset synthetic")
		 .arg(sgptorem.name_.buf()), return false );

    if ( !synthstobedisabled.isEmpty() )
    {
	uiString chgstr = toberemoved ? tr( "remove" ) : tr( "change" );
	uiString msg = tr("%1 will become undetiable as it is dependent on '%2'"
			  ".\n\nDo you want to %3 the synthetics?")
			  .arg(synthstobedisabled.getDispString())
			  .arg(sgptorem.name_.buf()).arg(chgstr);
	if ( !uiMSG().askGoOn(msg) )
	    return false;

	for ( int idx=0; idx<synthstobedisabled.size(); idx++ )
	{
	    const BufferString& synthnm = synthstobedisabled.get( idx );
	    synthDisabled.trigger( synthnm );
	}
    }

    return true;
}


void uiSynthGenDlg::removeSyntheticsCB( CallBacker* )
{
    if ( !prepareSyntheticToBeChanged(true) )
	return;

    const SynthGenParams& sgptorem = stratsynth_.genParams();
    synthRemoved.trigger( sgptorem.name_ );
    synthnmlb_->removeItem( synthnmlb_->indexOf(sgptorem.name_) );
}


void uiSynthGenDlg::updateFieldDisplay()
{
    SynthGenParams::SynthType synthtype =
	SynthGenParams::parseEnumSynthType( typefld_->text() );
    const bool psbased = synthtype == SynthGenParams::AngleStack ||
			 synthtype == SynthGenParams::AVOGradient;
    synthseis_->updateFieldDisplay();
    synthseis_->display( !psbased );
    psselfld_->display( psbased );
    if ( psbased )
	synthseis_->updateDisplayForPSBased();
    angleinpfld_->display( psbased );
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    updateFieldDisplay();
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
    NotifyStopper parschgstopper( synthseis_->parsChanged );
    NotifyStopper angparschgstopper( angleinpfld_->valuechanged );
    const SynthGenParams& genparams = stratsynth_.genParams();
    synthseis_->setWavelet( genparams.wvltnm_ );
    namefld_->setText( genparams.name_ );

    typefld_->setCurrentItem( SynthGenParams::toString(genparams.synthtype_) );

    if ( genparams.isPSBased() )
    {
	BufferStringSet psnms;
	getPSNames( psnms );
	psselfld_->box()->setEmpty();
	if ( psnms.isPresent(genparams.inpsynthnm_) ||
	     genparams.inpsynthnm_.isEmpty() )
	{
	    psselfld_->box()->addItems( psnms );
	    psselfld_->box()->setCurrentItem( genparams.inpsynthnm_ );
	    psselfld_->box()->setSensitive( true );
	}
	else
	{
	    psselfld_->box()->addItem( toUiString(genparams.inpsynthnm_) );
	    psselfld_->box()->setSensitive( false );
	}

	angleinpfld_->setValue( genparams.anglerg_ );
	updateFieldDisplay();
	return;
    }

    synthseis_->usePar( genparams.raypars_ );
    updateFieldDisplay();
}


bool uiSynthGenDlg::getFromScreen()
{
    const char* nm = namefld_->text();
    if ( !nm )
	mErrRet(tr("Please specify a valid name"),return false);

    stratsynth_.genParams().raypars_.setEmpty();

    SynthGenParams& genparams = stratsynth_.genParams();
    genparams.synthtype_ = SynthGenParams::parseEnumSynthType(typefld_->text());

    if ( genparams.isPSBased() )
    {
	SynthGenParams::SynthType synthtype = genparams.synthtype_;
	if ( psselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot generate an angle stack synthetics without any "
			"NMO corrected Prestack."), return false );

	if ( !psselfld_->box()->sensitive() )
	    mErrRet( tr("Cannot change synthetic data as the dependent prestack"
			" synthetic data has already been removed"),
			return false );

	SyntheticData* inppssd = stratsynth_.getSynthetic(
		psselfld_->box()->textOfItem(psselfld_->box()->currentItem()) );
	if ( !inppssd )
	    mErrRet( tr("Problem with Input Prestack synthetic data"),
		     return false);

	inppssd->fillGenParams( genparams );
	genparams.name_ = nm;
	genparams.synthtype_ = synthtype;
	genparams.inpsynthnm_ = inppssd->name();
	genparams.anglerg_ = angleinpfld_->getFInterval();
	return true;
    }

    synthseis_->fillPar( genparams.raypars_ );
    genparams.wvltnm_ = synthseis_->getWaveletName();
    genparams.name_ = namefld_->text();

    return true;
}


void uiSynthGenDlg::updateWaveletName()
{
    synthseis_->setWavelet( stratsynth_.genParams().wvltnm_ );
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


bool uiSynthGenDlg::acceptOK( CallBacker* )
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return true;

    if ( !prepareSyntheticToBeChanged(false) )
	return false;

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


void uiSynthGenDlg::genNewCB( CallBacker* )
{
    if ( !getFromScreen() ) return;

    if ( stratsynth_.genParams().name_ == SynthGenParams::sKeyInvalidInputPS() )
	mErrRet( tr("Please enter a different name"), return );

    if ( synthnmlb_->isPresent(stratsynth_.genParams().name_) )
    {
	uiString msg = tr("Synthectic data of name '%1' is already present. "
			  "Please choose a different name" )
		     .arg(stratsynth_.genParams().name_);
	mErrRet( msg, return );
    }

    genNewReq.trigger();
}
