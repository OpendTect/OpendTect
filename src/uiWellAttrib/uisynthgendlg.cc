/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uisynthgendlg.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uisynthseis.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "file.h"
#include "instantattrib.h"
#include "od_helpids.h"
#include "seistrctr.h"
#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "synthseis.h"


#define mErrRet(s,act) \
{ \
    uiMsgMainWinSetter mws( mainwin() ); \
    if ( !s.isEmpty() ) \
	uiMSG().error(s); \
    act; \
}

uiSynthParsGrp::uiSynthParsGrp( uiParent* p, StratSynth& gp )
    : uiGroup(p,"Synthetic Seismic Parameters")
    , stratsynth_(gp)
    , synthAdded(this)
    , synthChanged(this)
    , synthRenamed(this)
    , synthRemoved(this)
    , synthDisabled(this)
    , elPropSel(this)
{
    auto* leftgrp = new uiGroup( this, "left group" );
    auto* butgrp = new uiButtonGroup( leftgrp, "actions", OD::Horizontal );

    new uiToolButton( butgrp, "defraytraceprops",
		      tr("Specify inputs for synthetic creation"),
		      mCB(this,uiSynthParsGrp,elPropSelCB) );
    new uiToolButton( butgrp, "new", uiStrings::sNew(),
				     mCB(this,uiSynthParsGrp,newCB) );
    new uiToolButton( butgrp, "open", uiStrings::sOpen(),
				      mCB(this,uiSynthParsGrp,openCB) );
    new uiToolButton( butgrp, "save", uiStrings::sSave(),
				      mCB(this,uiSynthParsGrp,saveCB) );
    new uiToolButton( butgrp, "saveas", uiStrings::sSaveAs(),
				      mCB(this,uiSynthParsGrp,saveAsCB) );
    /*TODO: ensure we can open/save not only directly the synthetic data
      parameters, but also the ones saved within SynthRock simulations */

    auto* syntlistgrp = new uiGroup( leftgrp, "Synthetics List" );
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Synthetics"),
			 uiListBox::AboveMid );
    synthnmlb_ = new uiListBox( syntlistgrp, su );
    synthnmlb_->setHSzPol( uiObject::SmallVar );
    mAttachCB( synthnmlb_->selectionChanged, uiSynthParsGrp::newSynthSelCB );
    updatefld_ = new uiPushButton( syntlistgrp, tr("Update selected"),
			mCB(this,uiSynthParsGrp,updateSyntheticsCB), true );
    updatefld_->attach( leftAlignedBelow, synthnmlb_ );
    removefld_ = new uiPushButton( syntlistgrp, tr("Remove selected"),
			mCB(this,uiSynthParsGrp,removeSyntheticsCB), true );
    removefld_->attach( rightOf, updatefld_ );

    auto* rightgrp = new uiGroup( this, "Parameter Group" );
    rightgrp->setStretch( 1, 1 );

    uiMultiSynthSeisSel::Setup sssu;
    sssu.compact( true );
    synthselgrp_ = new uiFullSynthSeisSel( rightgrp, sssu );
    mAttachCB( synthselgrp_->selectionChanged, uiSynthParsGrp::typeChgCB );
    mAttachCB( synthselgrp_->parsChanged, uiSynthParsGrp::parsChangedCB );
    mAttachCB( synthselgrp_->nameChanged, uiSynthParsGrp::nameChangedCB );

    addnewfld_ = new uiPushButton( rightgrp, tr("Add as new"),
			mCB(this,uiSynthParsGrp,addSyntheticsCB), true );
    addnewfld_->attach( alignedBelow, synthselgrp_ );
    rightgrp->setHAlignObj( synthselgrp_->attachObj() );

    auto* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
    butgrp->attach( leftAlignedAbove, syntlistgrp );

    mAttachCB( postFinalise(), uiSynthParsGrp::initGrp );
}


uiSynthParsGrp::~uiSynthParsGrp()
{
    detachAllNotifiers();
}


void uiSynthParsGrp::initGrp( CallBacker* )
{
    NotifyStopper ns( synthnmlb_->selectionChanged );
    const BufferString cursel( synthnmlb_->isEmpty() ? ""
						     : synthnmlb_->getText() );
    synthnmlb_->setEmpty();
    BufferStringSet synthnms;
    for ( const auto* sd : stratsynth_.synthetics() )
    {
	if ( !sd->isStratProp() )
	    synthnms.add( sd->name().buf() );
    }

    synthnmlb_->addItems( synthnms );
    if ( !synthnmlb_->isEmpty() )
    {
	if ( !cursel.isEmpty() && synthnms.isPresent(cursel) )
	    synthnmlb_->setCurrentItem( cursel.buf() );
	else
	    synthnmlb_->setCurrentItem( 0 );

	forwardInputNames();
    }

    ns.enableNotification();
    newSynthSelCB( nullptr );
    namechanged_ = parschanged_ = false;
    //Keep last, as we do not want earlier notifications:
    mAttachCB( synthselgrp_->parsChanged, uiSynthParsGrp::parsChangedCB );
}


void uiSynthParsGrp::forwardInputNames()
{
    BufferStringSet psnms, inpnms;
    getPSNames( psnms ); getInpNames( inpnms );
    if ( !psnms.isEmpty() )
	synthselgrp_->manPSSynth( psnms.cat("`"), true );
    if ( !inpnms.isEmpty() )
	synthselgrp_->manInpSynth( inpnms.cat("`"), true );
}


void uiSynthParsGrp::getPSNames( BufferStringSet& synthnms )
{
    synthnms.erase();
    stratsynth_.getSyntheticNames( synthnms, SynthGenParams::PreStack );
    for ( int idx=synthnms.size()-1; idx>=0; idx-- )
    {
	const SyntheticData* sd = stratsynth_.getSynthetic( synthnms.get(idx) );
	if ( sd && sd->synthGenDP().isCorrected() )
	    synthnms.add( sd->name() );
    }
}


void uiSynthParsGrp::getInpNames( BufferStringSet& synthnms )
{
    synthnms.erase();
    for ( int synthidx=0; synthidx<stratsynth_.nrSynthetics(); synthidx++ )
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( synthidx );
	if ( sd && sd->getGenParams().canBeAttributeInput() )
	    synthnms.add( sd->name() );
    }
}


bool uiSynthParsGrp::prepareSyntheticToBeRemoved()
{
    if ( synthnmlb_->size()==1 )
	mErrRet( tr("Cannot remove all synthetics"), return false );

    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	mErrRet( tr("No synthetic selected"), return false );

    const BufferString synthtochgnm( synthnmlb_->getText() );
    const SyntheticData* sdtochg = stratsynth_.getSynthetic( synthtochgnm );
    if ( !sdtochg )
	mErrRet( tr("Cannot find synthetic data '%1'").arg(synthtochgnm),
		 return false );

    BufferStringSet synthstobedisabled;
    const SynthGenParams& sgptorem = sdtochg->getGenParams();
    for ( const auto* sd : stratsynth_.synthetics() )
    {
	if ( !sd->isAngleStack() && !sd->isAVOGradient() && !sd->isAttribute() )
	    continue;

	const SynthGenParams& sgp = sd->getGenParams();
	if ( sgp.inpsynthnm_ == sgptorem.name_ &&
	     ((sgptorem.canBeAttributeInput() && sgp.isAttribute()) ||
	      (sgptorem.isPreStack() && sgp.isPSBased())) )
	    synthstobedisabled.add( sgp.name_ );
    }

    if ( !synthstobedisabled.isEmpty() )
    {
	uiString msg = tr("%1 will become uneditable as it is dependent on "
			  "'%2'.\n\nDo you want to remove the synthetics?")
			  .arg(synthstobedisabled.getDispString())
			  .arg(sgptorem.name_.buf());
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


bool uiSynthParsGrp::checkSyntheticName( const char* nm, bool isupdate )
{
    if ( FixedString(nm) == SynthGenParams::sKeyInvalidInputPS() )
	mErrRet( tr("Please enter a different name"), return false);

    if ( !isupdate && synthnmlb_->isPresent(nm) )
    {
	uiString msg = tr("Synthetic data of name '%1' is already present. "
			  "Please choose a different name" ).arg( nm );
	mErrRet( msg, return false );
    }

    return true;
}


bool uiSynthParsGrp::doAddSynthetic( const SynthGenParams& sgp, bool isupdate )
{
    checkSyntheticName( sgp.name_, isupdate );

    MouseCursorChanger mcchger( MouseCursor::Wait );
    const SyntheticData* sd = stratsynth_.addSynthetic( sgp );
    if ( !sd )
	mErrRet( stratsynth_.errMsg(), return false )

    return true;
}


void uiSynthParsGrp::newSynthSelCB( CallBacker* )
{
    putToScreen();
    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( synthnmlb_->size() < 1 );
}


void uiSynthParsGrp::updateSyntheticsCB( CallBacker* )
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return;

    const bool onlynmchanged = namechanged_ && !parschanged_;
    const BufferString synthtochgnm( synthnmlb_->getText() );
    ConstRefMan<SyntheticData> sdtochg =
			       stratsynth_.getSynthetic( synthtochgnm );
    const SynthGenParams& sdtochgsgp = sdtochg->getGenParams();

    SynthGenParams cursgp( sdtochgsgp );
    IOPar iop;
    synthselgrp_->fillPar( iop );
    cursgp.usePar( iop );
    if ( cursgp == sdtochgsgp )
    {
	updatefld_->setSensitive( false );
	addnewfld_->setSensitive( false );
	namechanged_ = parschanged_ = false;
	return;
    }

    const BufferString synthname( sdtochg->name() );
    const BufferString& newsynthnm = cursgp.name_;
    if ( sdtochgsgp.canBeAttributeInput() )
	synthselgrp_->manInpSynth( synthtochgnm, false );
    else if ( sdtochgsgp.isPreStack() )
	synthselgrp_->manPSSynth( synthtochgnm, false );

    sdtochg = nullptr;

    if ( (onlynmchanged && !stratsynth_.updateSyntheticName(synthname,
							   newsynthnm)) ||
	 (!onlynmchanged && !stratsynth_.updateSynthetic(synthname,cursgp)) )
    {
	mErrRet( stratsynth_.errMsg(), return )
    }

    forwardInputNames();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd || sd->isStratProp() )
	    continue;

	synthnmlb_->setItemText( idx, sd->name() );
    }

    updatefld_->setSensitive( false );
    addnewfld_->setSensitive( false );
    namechanged_ = parschanged_ = false;
    const BufferStringSet nms( synthname, newsynthnm );
    if ( onlynmchanged )
	synthRenamed.trigger( nms );
    else
	synthChanged.trigger( nms );
}


void uiSynthParsGrp::removeSyntheticsCB( CallBacker* )
{
    if ( !prepareSyntheticToBeRemoved() )
	return;

    const BufferString synthtoremnm( synthnmlb_->getText() );
    const SyntheticData* sd = stratsynth_.getSynthetic( synthtoremnm );
    if ( !sd )
    {
	synthnmlb_->removeItem( synthnmlb_->indexOf(synthtoremnm) );
	removefld_->setSensitive( synthnmlb_->size() > 1 );
	return;
    }

    const SynthGenParams sgp = sd->getGenParams();
    stratsynth_.removeSynthetic( synthtoremnm );
    synthRemoved.trigger( synthtoremnm );

    synthnmlb_->removeItem( synthnmlb_->indexOf(synthtoremnm) );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    if ( sgp.canBeAttributeInput() )
	synthselgrp_->manInpSynth( synthtoremnm, false );
    else if ( sgp.isPreStack() )
	synthselgrp_->manPSSynth( synthtoremnm, false );

    namechanged_ = parschanged_ = false;
}


void uiSynthParsGrp::addSyntheticsCB( CallBacker* )
{
    SynthGenParams sgp;
    if ( !getFromScreen(sgp) )
	return;

    if ( sgp.name_.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnterValidName() );
	return;
    }
    else if ( synthnmlb_->isPresent(sgp.name_) )
    {
	uiMSG().error( tr("Another synthetic dataset exists with that name") );
	return;
    }

    if ( sgp.isAttribute() )
    {
	BufferStringSet attribs;
	synthselgrp_->getChosenInstantAttribs( attribs );
	for ( const auto* attrib : attribs )
	{
	    Attrib::Instantaneous::parseEnum( *attrib, sgp.attribtype_ );
	    sgp.createName( sgp.name_ );
	    if ( !checkSyntheticName(sgp.name_) )
		return;
	}

	MouseCursorChanger mcchger( MouseCursor::Wait );
	if ( !stratsynth_.addInstAttribSynthetics(attribs,sgp) )
	    mErrRet( stratsynth_.errMsg(), return );

	for ( const auto* attrib : attribs )
	{
	    Attrib::Instantaneous::parseEnum( *attrib, sgp.attribtype_ );
	    sgp.createName( sgp.name_ );
	    synthAdded.trigger( sgp.name_ );
	    NotifyStopper ns( synthnmlb_->selectionChanged );
	    synthnmlb_->addItem( sgp.name_ );
	}
	synthnmlb_->selectionChanged.trigger();
    }
    else
    {
	if ( !doAddSynthetic(sgp) )
	    return;

	const BufferString& newsynthnm = sgp.name_;
	synthAdded.trigger( newsynthnm );
	NotifyStopper ns( synthnmlb_->selectionChanged );
	synthnmlb_->addItem( newsynthnm );
	if ( sgp.canBeAttributeInput() )
	    synthselgrp_->manInpSynth( newsynthnm, true );
	else if ( sgp.isPreStack() )
	    synthselgrp_->manPSSynth( newsynthnm, true );
    }

    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( false );
    namechanged_ = parschanged_ = false;
}


void uiSynthParsGrp::fillPar( IOPar& par ) const
{
    stratsynth_.fillPar( par );
}


bool uiSynthParsGrp::usePar( const IOPar& par )
{
    MouseCursorChanger mc( MouseCursor::Wait );
    const bool res = stratsynth_.usePar( par );
    if ( !res )
	return false;

    for ( int idx=synthnmlb_->size()-1; idx>=0; idx-- )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	synthRemoved.trigger( synthnm );
    }

    initGrp( nullptr );
    for ( int idx=0; idx<synthnmlb_->size(); idx++ )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	const SyntheticData* sd = stratsynth_.getSynthetic( synthnm.buf() );
	if ( sd )
	    synthAdded.trigger( synthnm );
    }

    return res;
}


bool uiSynthParsGrp::confirmSave()
{
    const int dosave = uiMSG().askSave( tr("%1 are not saved. Save now?")
					.arg( uiStrings::sParameter(mPlural)));
    if ( dosave == -1 )
	return false;
    else if ( dosave == 1 )
	saveCB( nullptr );
    return true;
}


void uiSynthParsGrp::elPropSelCB( CallBacker* )
{
    elPropSel.trigger();
}


void uiSynthParsGrp::newCB( CallBacker* )
{
    if ( !confirmSave() )
	return;

    const SyntheticData* defsd = stratsynth_.addDefaultSynthetic();

    bool newtriggered = false;
    for ( int idx=synthnmlb_->size()-1; idx>=0; idx-- )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	if ( stratsynth_.removeSynthetic(synthnm) )
	{
	    if ( defsd && synthnm == defsd->name() )
	    {
		const BufferStringSet synthnms( synthnm, synthnm );
		synthChanged.trigger( synthnms );
		newtriggered = true;
	    }
	    else
		synthRemoved.trigger( synthnm );
	}
    }

    if ( defsd && !newtriggered )
	synthAdded.trigger( BufferString(defsd->name()) );

    initGrp( nullptr );
}


void uiSynthParsGrp::openCB( CallBacker* )
{
    if ( !confirmSave() )
	return;

    IOPar previouspar, par;
    const IOObjContext ctio( mIOObjContext(SyntheticDataPars) );
    uiIOObjSelDlg dlg( this, uiStrings::phrOpen( tr("synthetic parameters") ),
		       ctio );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return;

    if ( !par.read(dlg.ioObj()->mainFileName(),
		   mTranslGroupName(SyntheticDataPars)) )
	return;

    MouseCursorChanger mc( MouseCursor::Wait );
    fillPar( previouspar );
    stratsynth_.clearSynthetics( true );
    if ( usePar(par) )
	lastsavedfnm_.set( dlg.ioObj()->mainFileName() );
    else
    { //Try to restore the previous state
	stratsynth_.clearSynthetics( true );
	usePar( previouspar );
    }
}


void uiSynthParsGrp::saveCB( CallBacker* )
{
    if ( !lastsavedfnm_.isEmpty() && File::exists(lastsavedfnm_.str()) )
    {
	doSave( lastsavedfnm_.str() );
	return;
    }

    saveAsCB( nullptr );
}


void uiSynthParsGrp::saveAsCB( CallBacker* )
{
    IOObjContext ctio( mIOObjContext(SyntheticDataPars) );
    ctio.forread_ = false;
    uiIOObjSelDlg dlg( this, uiStrings::phrSave( tr("synthetic parameters") ),
		       ctio );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return;

    doSave( dlg.ioObj()->mainFileName() );
}


bool uiSynthParsGrp::doSave( const char* fnm )
{
    IOPar par;
    fillPar( par );
    const bool res = par.write( fnm, mTranslGroupName(SyntheticDataPars) );
    lastsavedfnm_.set( fnm );
    return res;
}


void uiSynthParsGrp::typeChgCB( CallBacker* cb )
{
    parsChangedCB( cb );
}


void uiSynthParsGrp::parsChangedCB( CallBacker* )
{
    updatefld_->setSensitive( !synthnmlb_->isEmpty() );
    addnewfld_->setSensitive( true );
    parschanged_ = true;
}


void uiSynthParsGrp::nameChangedCB( CallBacker* cb )
{
    updatefld_->setSensitive( !synthnmlb_->isEmpty() );
    addnewfld_->setSensitive( true );
    namechanged_ = true;
}


void uiSynthParsGrp::putToScreen()
{
    NotifyStopper nssel( synthselgrp_->selectionChanged );
    NotifyStopper nspars( synthselgrp_->parsChanged );

    IOPar par;
    const BufferString synthnm( synthnmlb_->getText() );
    const SyntheticData* sd = stratsynth_.getSynthetic( synthnm );
    if ( sd )
	sd->getGenParams().fillPar( par );
    else
    {
	const SynthGenParams::SynthType synthtype =
		SynthGenParams::parseEnumSynthType( synthselgrp_->getType() );
	SynthGenParams sgp( synthtype );
	if ( sgp.isRawOutput() )
	{
	    const BufferString wvltnm( synthselgrp_->getWaveletName() );
	    if ( !wvltnm.isEmpty() )
	    {
		sgp.setWavelet( wvltnm );
		sgp.createName( sgp.name_ );
	    }
	}
	sgp.fillPar( par );
    }

    synthselgrp_->usePar( par );
}


bool uiSynthParsGrp::getFromScreen( SynthGenParams& sgp )
{
    const uiRetVal uirv = synthselgrp_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv,return false);

    IOPar iop;
    synthselgrp_->fillPar( iop );

    const SynthGenParams::SynthType synthtype =
		SynthGenParams::parseEnumSynthType( synthselgrp_->getType() );
    sgp = SynthGenParams( synthtype );
    sgp.usePar( iop );
    if ( iop.isEmpty() )
	return false;
    else if ( sgp.needsInput() )
    {
	const SyntheticData* inppssd = stratsynth_.getSynthetic(
							sgp.inpsynthnm_ );
	if ( !inppssd )
	    mErrRet(tr("Problem with Input synthetic data"),return false);
    }

    return true;
}



//uiSynthGenDlg

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp )
    : uiDialog(p,uiDialog::Setup(tr("Specify Synthetic Parameters"),
				mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
				.modal(false))
{
    setForceFinalise( true );
    setCtrlStyle( CloseOnly );

    uisynthparsgrp_ = new uiSynthParsGrp( this, gp );
}


uiSynthGenDlg::~uiSynthGenDlg()
{
}
