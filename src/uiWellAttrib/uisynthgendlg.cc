/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisynthgendlg.h"

#include "uibuttongroup.h"
#include "uielasticpropsel.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uistratsynthexport.h"
#include "uisynthtorealscale.h"
#include "uisynthseis.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "file.h"
#include "instantattrib.h"
#include "od_helpids.h"
#include "seistrctr.h"
#include "stratlayermodel.h"
#include "stratsynth.h"
#include "survinfo.h"
#include "wavelet.h"


#define mErrRet(s,act) \
{ \
    uiMsgMainWinSetter mws( mainwin() ); \
    if ( !s.isEmpty() ) \
	uiMSG().error(s); \
    act; \
}

uiSynthParsGrp::uiSynthParsGrp( uiParent* p, StratSynth::DataMgr& gp )
    : uiGroup(p,"Synthetic Seismic Parameters")
    , synthAdded(this)
    , synthSelected(this)
    , stratsynth_(gp)
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
    new uiToolButton( butgrp, "export",
			    uiStrings::phrExport( tr("Synthetic DataSet(s)")),
				      mCB(this,uiSynthParsGrp,expSynthCB) );
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
    scalefld_ = new uiPushButton( syntlistgrp, tr("Scale"),
			mCB(this,uiSynthParsGrp,scaleSyntheticsCB), true );
    scalefld_->attach( rightOf, removefld_ );

    auto* rightgrp = new uiGroup( this, "Parameter Group" );
    rightgrp->setStretch( 1, 1 );

    uiMultiSynthSeisSel::Setup sssu;
    sssu.compact( true );
#ifndef __debug__
    sssu.withelasticgather( false ); //TODO: enable later if needed
#endif
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

    mAttachCB( postFinalize(), uiSynthParsGrp::initGrp );
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
    stratsynth_.getNames( synthnms, StratSynth::DataMgr::NoProps );
    synthnmlb_->addItems( synthnms );
    if ( !synthnmlb_->isEmpty() )
    {
	if ( !cursel.isEmpty() && synthnms.isPresent(cursel) )
	    synthnmlb_->setCurrentItem( cursel.buf() );
	else
	    synthnmlb_->setCurrentItem( 0 );

	forwardInputNames( nullptr );
    }

    ns.enableNotification();
    newSynthSelCB( nullptr );
    namechanged_ = parschanged_ = false;
    //Keep last, as we do not want earlier notifications:
    mAttachCB( stratsynth_.newWvltUsed, uiSynthParsGrp::newWvltCB );
    mAttachCB( stratsynth_.wvltScalingDone, uiSynthParsGrp::scalingDoneCB );
    mAttachCB( synthselgrp_->parsChanged, uiSynthParsGrp::parsChangedCB );
}


void uiSynthParsGrp::forwardInputNames( const SynthGenParams* sgp )
{
    if ( !sgp || sgp->isPreStack() )
    {
	BufferStringSet psnms;
	getPSNames( psnms );
	if ( !psnms.isEmpty() )
	    synthselgrp_->manPSSynth( psnms );
    }

    if ( sgp )
    {
	BufferStringSet inpnms;
	if ( sgp->isAttribute() )
	    getAttributeInpNames( inpnms );
	else if ( sgp->isFiltered() )
	    getFilteringInpNames( inpnms, sgp->isFilteredSynthetic() );

	if ( !inpnms.isEmpty() )
	    synthselgrp_->manInpSynth( inpnms );
    }
}


void uiSynthParsGrp::getPSNames( BufferStringSet& synthnms )
{
    synthnms.setEmpty(); // Read from screen order
    for ( int idx=0; idx<synthnmlb_->size(); idx++ )
    {
	const BufferString dsnm( synthnmlb_->textOfItem(idx) );
	const SynthID id = stratsynth_.find( dsnm );
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->isPreStack() && sgp->isCorrected() )
	    synthnms.add( sgp->name_ );
    }

    TypeSet<SynthID> ids;
    stratsynth_.getIDs( ids, StratSynth::DataMgr::OnlyPSBased );
    for ( const auto& id : ids )
    {
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->inpsynthnm_ == SynthGenParams::sKeyInvalidInputPS() )
	{
	    synthnms.add( SynthGenParams::sKeyInvalidInputPS() );
	    break;
	}
    }
}


void uiSynthParsGrp::getAttributeInpNames( BufferStringSet& synthnms )
{
    synthnms.setEmpty();
    for ( int idx=0; idx<synthnmlb_->size(); idx++ )
    {
	const BufferString dsnm( synthnmlb_->textOfItem(idx) );
	const SynthID id = stratsynth_.find( dsnm );
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->canBeAttributeInput() )
	    synthnms.add( sgp->name_ );
    }

    TypeSet<SynthID> ids;
    stratsynth_.getIDs( ids, StratSynth::DataMgr::OnlyAttrib );
    for ( const auto& id : ids )
    {
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->inpsynthnm_ == SynthGenParams::sKeyInvalidInputPS() )
	{
	    synthnms.add( SynthGenParams::sKeyInvalidInputPS() );
	    break;
	}
    }
}


void uiSynthParsGrp::getFilteringInpNames( BufferStringSet& synthnms,
					   bool issynthetic )
{
    synthnms.setEmpty();
    if ( issynthetic )
    {
	stratsynth_.getNames( synthnms, StratSynth::DataMgr::OnlyPSBased );
	stratsynth_.getNames( synthnms, StratSynth::DataMgr::OnlyZO );
	stratsynth_.getNames( synthnms, StratSynth::DataMgr::OnlyAttrib );
    }
    else
    {
	stratsynth_.getNames( synthnms, StratSynth::DataMgr::OnlyProps );
	if ( synthnms.isEmpty() )
	{
	    stratsynth_.addPropertySynthetics();
	    stratsynth_.getNames( synthnms, StratSynth::DataMgr::OnlyProps );
	}
    }

    TypeSet<SynthID> ids;
    stratsynth_.getIDs( ids, StratSynth::DataMgr::OnlyFilter );
    for ( const auto& id : ids )
    {
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->inpsynthnm_ == SynthGenParams::sKeyInvalidInputPS() )
	{
	    synthnms.add( SynthGenParams::sKeyInvalidInputPS() );
	    break;
	}
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
    const SynthID sdidtochg = stratsynth_.find( synthtochgnm );
    const SynthGenParams* sgptorem = stratsynth_.getGenParams( sdidtochg );
    if ( !sgptorem )
	mErrRet( tr("Cannot find synthetic data '%1'").arg(synthtochgnm),
		 return false );

    TypeSet<SynthID> idswithinput, idstobedisabled;
    stratsynth_.getIDs( idswithinput, StratSynth::DataMgr::OnlyWithInput );
    for ( const auto& id : idswithinput )
    {
	const SynthGenParams* sgp = stratsynth_.getGenParams( id );
	if ( sgp && sgp->inpsynthnm_ == synthtochgnm &&
	     ((sgptorem->canBeAttributeInput() && sgp->isAttribute()) ||
	      (sgptorem->isPreStack() && sgp->isPSBased())) )
	    idstobedisabled += id;
    }

    for ( const auto& id : idstobedisabled )
    {
	bool dontaskagain = false;
	const BufferString dsnm = stratsynth_.nameOf( id );
	uiString msg = tr("%1 will become uneditable as it is dependent on "
			  "'%2'.\n\nDo you want to remove the synthetics?")
			  .arg(dsnm).arg(synthtochgnm);
	uiMsgMainWinSetter mws( mainwin() );
	if ( !dontaskagain && !uiMSG().askGoOn(msg,true,&dontaskagain) )
	    return false;
    }

    stratsynth_.disableSynthetic( idstobedisabled );

    return true;
}


bool uiSynthParsGrp::checkSyntheticPars( const SynthGenParams& sgp,
					 bool isupdate )
{
    if ( sgp.name_.isEmpty() )
    {
	mErrRet( uiStrings::phrEnterValidName(), return false );
    }
    else if ( !sgp.isOK() )
    {
	const uiString msg = tr("Invalid synthetic generation parameters");
	mErrRet( msg, return false );
    }

    if ( !isupdate && synthnmlb_->isPresent(sgp.name_) )
    {
	const uiString msg =
		tr("Synthetic data of name '%1' is already present. "
		   "Please choose a different name" ).arg( sgp.name_ );
	mErrRet( msg, return false );
    }

    return true;
}


void uiSynthParsGrp::newSynthSelCB( CallBacker* )
{
    putToScreen();
    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( synthnmlb_->size() < 1 );
    needScaleCB( nullptr );
}


void uiSynthParsGrp::updateSyntheticsCB( CallBacker* )
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return;

    const bool onlynmchanged = namechanged_ && !parschanged_;
    const BufferString synthtochgnm( synthnmlb_->getText() );
    const SynthID sdtochgid = stratsynth_.find( synthtochgnm );
    const SynthGenParams* sdtochgsgp = stratsynth_.getGenParams( sdtochgid );
    if ( !sdtochgsgp )
    {
	pErrMsg("Updating non-registered synthetic");
	return;
    }

    SynthGenParams cursgp( *sdtochgsgp );
    IOPar iop;
    synthselgrp_->fillPar( iop );
    cursgp.usePar( iop );
    if ( cursgp == *sdtochgsgp )
    {
	updatefld_->setSensitive( false );
	addnewfld_->setSensitive( false );
	namechanged_ = parschanged_ = false;
	return;
    }
    else if ( !checkSyntheticPars(cursgp,true) )
	return;

    const BufferString& newsynthnm = cursgp.name_;
    if ( (onlynmchanged && !stratsynth_.updateSyntheticName(sdtochgid,
							    newsynthnm)) ||
	 (!onlynmchanged && !stratsynth_.updateSynthetic(sdtochgid,cursgp)) )
    {
	mErrRet( stratsynth_.errMsg(), return )
    }

    BufferStringSet synthnms;
    stratsynth_.getNames( synthnms, StratSynth::DataMgr::NoProps );
    for ( int idx=0; idx<synthnms.size(); idx++ )
	synthnmlb_->setItemText( idx, synthnms.get(idx).buf() );

    forwardInputNames( nullptr );
    synthAdded.trigger( sdtochgid );
    if ( !onlynmchanged )
	needScaleCB( nullptr );

    updatefld_->setSensitive( false );
    addnewfld_->setSensitive( false );
    namechanged_ = parschanged_ = false;
}


void uiSynthParsGrp::removeSyntheticsCB( CallBacker* )
{
    if ( !prepareSyntheticToBeRemoved() )
	return;

    const BufferString synthtoremnm( synthnmlb_->getText() );
    const SynthID sdid = stratsynth_.find( synthtoremnm );
    if ( !sdid.isValid() )
	return;

    stratsynth_.removeSynthetic( sdid );
    synthnmlb_->removeItem( synthnmlb_->indexOf(synthtoremnm) );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    forwardInputNames( nullptr );
    needScaleCB( nullptr );
    namechanged_ = parschanged_ = false;
}


void uiSynthParsGrp::needScaleCB( CallBacker* )
{
    if ( synthnmlb_->isEmpty() )
    {
	scalefld_->setSensitive( false );
	scalefld_->setText( tr("Scale") );
    }
    else
    {
	const BufferString synthnm( synthnmlb_->getText() );
	TypeSet<MultiID> wvltids;
	bool needscale = stratsynth_.getUnscaledSynthetics( nullptr, &wvltids );
	if ( needscale )
	{
	    SynthGenParams sgp;
	    if ( getFromScreen(sgp) )
	    {
		const MultiID wvltid = sgp.getWaveletID();
		needscale = wvltids.isPresent( wvltid );
	    }
	}

	scalefld_->setSensitive( true );
	scalefld_->setText( needscale ? tr("Scale") : tr("Re-Scale") );
    }
}


void uiSynthParsGrp::scaleSyntheticsCB( CallBacker* )
{
    SynthGenParams sgp;
    if ( !getFromScreen(sgp) )
	return;

    const MultiID wvltid = sgp.getWaveletID();
    TypeSet<MultiID> toscalewvltids; toscalewvltids += wvltid;
    const ScaleRes res = doScaling( this, stratsynth_, toscalewvltids );
    if ( isOK(res) && res != IGNORED )
    {
	scalefld_->setSensitive( true );
	scalefld_->setText( tr("Re-scale") );
    }
}


void uiSynthParsGrp::newWvltCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

     mCBCapsuleUnpack(const MultiID&,wvltid,cb);
     synthselgrp_->ensureHasWavelet( wvltid );
}


void uiSynthParsGrp::scalingDoneCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const BufferStringSet&,synthnms,cb);
    if ( synthnms.size() != 2 )
	return;

    const BufferString& oldnm = *synthnms.first();
    if ( !synthnmlb_->isPresent(oldnm) )
	return;

    const int idx = synthnmlb_->indexOf( oldnm.buf() );
    synthnmlb_->setItemText( idx, synthnms.last()->buf() );
    initGrp( nullptr );
}


bool uiSynthParsGrp::isOK( ScaleRes res )
{
    return res == ALLDONE || res == IGNORED || res == MARKED || res == SCALED;
}


uiSynthParsGrp::ScaleRes uiSynthParsGrp::checkUnscaledWavelets( uiParent* p,
						StratSynth::DataMgr& synthmgr )
{
    RefObjectSet<const SyntheticData> unscaledsynths;
    TypeSet<MultiID> unscaledwvlts;
    if ( !synthmgr.getUnscaledSynthetics(&unscaledsynths,&unscaledwvlts) )
    {
	static bool dontshowallscaled = false;
	if ( !dontshowallscaled )
	{
	    dontshowallscaled =
		uiMSG().message( tr("All synthetics use scaled wavelets"),
		    uiString::empty(), uiString::empty(), true );
	}

	return ALLDONE;
    }

    BufferStringSet opts;
    opts.add( "[Start tool]: Start the wavelet scaling dialog" );
    opts.add( "[Mark scaled]: The wavelets amplitude are already compatible "
	      "with the seismic data" );
    opts.add( "[Ignore]: I will not use scaling-sensitive operations" );
    uiGetChoice dlg( p, opts,
	    tr("Some wavelets seem to be unscaled.\n"
		"For most purposes, you will need a scaled wavelet.\n"), true );
    dlg.setHelpKey( mODHelpKey(mStratLayerModelcheckUnscaledWaveletHelpID));
    dlg.go();
    const int choice = dlg.choice();
    if ( choice < 0 )
	return CANCELLED;
    else if ( choice == 2 )
	return IGNORED;
    else if ( choice == 1 )
    {
	for ( const auto& wvltid : unscaledwvlts )
	    Wavelet::markScaled( wvltid );
	return MARKED;
    }

    return doScaling( p, synthmgr, unscaledwvlts );
}


uiSynthParsGrp::ScaleRes uiSynthParsGrp::doScaling( uiParent* p,
					StratSynth::DataMgr& synthmgr,
					const TypeSet<MultiID>& unscaledwvlts )
{
    ScaleRes res = SCALED;
    for ( const auto& wvltid : unscaledwvlts )
    {
	MultiID scaledwvltid;
	if ( !haveUserScaleWavelet(p,synthmgr,wvltid,scaledwvltid) )
	{
	    res = SCALEERROR;
	    continue;
	}

	synthmgr.updateWavelet( wvltid, scaledwvltid );
    }

    return res;
}


bool uiSynthParsGrp::haveUserScaleWavelet( uiParent* p,
					   const StratSynth::DataMgr& synthmgr,
					   const MultiID& wvltid,
					   MultiID& scaledwvltid )
{
    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	const int res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
	if ( res < 0 )
	    return false;

	is2d = res == 1;
    }

    uiSynthToRealScale dlg( p, is2d, synthmgr, wvltid );
    if ( !dlg.go() )
	return false;

    const MultiID& mid = dlg.selWvltID();
    if ( mid.isUdf() )
	return false;

    scaledwvltid = mid;
    return true;
}


void uiSynthParsGrp::addSyntheticsCB( CallBacker* )
{
    SynthGenParams sgp;
    if ( !getFromScreen(sgp) || !checkSyntheticPars(sgp,false) )
	return;

    if ( sgp.isAttribute() )
    {
	const SynthID inpid = stratsynth_.find( sgp.inpsynthnm_);
	if ( !inpid.isValid() )
	    return;

	BufferStringSet attribs;
	synthselgrp_->getChosenInstantAttribs( attribs );
	TypeSet<Attrib::Instantaneous::OutType> attribtypes;
	for ( const auto* attrib : attribs )
	{
	    Attrib::Instantaneous::parseEnum( attrib->buf(), sgp.attribtype_ );
	    attribtypes += sgp.attribtype_;
	    BufferString attribnm;
	    sgp.createName( attribnm );
	    if ( !checkSyntheticPars(sgp,false) )
		return;
	}

	MouseCursorChanger mcchger( MouseCursor::Wait );
	TypeSet<SynthID> attribids;
	if ( !stratsynth_.addInstAttribSynthetics(inpid,attribtypes,attribids) )
	    mErrRet( stratsynth_.errMsg(), return );

	for ( const auto& id : attribids )
	{
	    const BufferString attribnm = stratsynth_.nameOf( id );
	    NotifyStopper ns( synthnmlb_->selectionChanged );
	    synthnmlb_->addItem( attribnm );
	    forwardInputNames( &sgp );
	    synthAdded.trigger( id );
	}
    }
    else if ( sgp.isFiltered() )
    {
	const SynthID inpid = stratsynth_.find( sgp.inpsynthnm_);
	if ( !inpid.isValid() )
	    return;

	BufferString filtname;
	sgp.createName( filtname );
	if ( !checkSyntheticPars(sgp,false) )
	    return;

	const SynthID sdid = stratsynth_.addSynthetic( sgp );
	if ( !sdid.isValid() )
	    mErrRet( stratsynth_.errMsg(), return )

	const BufferString& newsynthnm = stratsynth_.nameOf( sdid );
	NotifyStopper ns( synthnmlb_->selectionChanged );
	synthnmlb_->addItem( newsynthnm );
	forwardInputNames( &sgp );
	synthAdded.trigger( sdid );
    }
    else
    {
	NotifyStopper nselprop( stratsynth_.elPropSelChanged );
	const SynthID sdid = stratsynth_.addSynthetic( sgp );
	if ( !sdid.isValid() )
	    mErrRet( stratsynth_.errMsg(), return )

	const BufferString& newsynthnm = sgp.name_;
	NotifyStopper ns( synthnmlb_->selectionChanged );
	synthnmlb_->addItem( newsynthnm );
	forwardInputNames( &sgp );
	synthAdded.trigger( sdid );
    }

    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( false );
    needScaleCB( nullptr );
    namechanged_ = parschanged_ = false;
}


void uiSynthParsGrp::fillPar( IOPar& iop ) const
{
    IOPar par;
    stratsynth_.fillPar( par );
    PtrMan<IOPar> synthpars =
			par.subselect( StratSynth::DataMgr::sKeySynthetics() );
    if ( synthpars )
	iop.merge( *synthpars.ptr() );
}


bool uiSynthParsGrp::usePar( const IOPar& par )
{
    MouseCursorChanger mc( MouseCursor::Wait );
    const bool res = stratsynth_.usePar( par );
    if ( !res )
	return false;

    initGrp( nullptr );
    const BufferString dsnm = synthnmlb_->getText();
    const SynthID id = stratsynth_.find( dsnm );
    if ( id.isValid() )
	synthSelected.trigger( id );

    return id.isValid();
}


void uiSynthParsGrp::reset()
{
    newCB( nullptr );
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
    const Strat::LayerModel& lm = stratsynth_.layerModel();
    const ElasticPropSelection& origelpropsel = lm.elasticPropSel();
    ElasticPropSelection elpropsel = origelpropsel;
    uiElasticPropSelDlg dlg( this, lm.propertyRefs(), elpropsel );
    if ( !dlg.go() || elpropsel == origelpropsel )
	return;

    uiString msg;
    if ( !stratsynth_.checkElasticPropSel(elpropsel,nullptr,&msg) )
    {
	uiMSG().error( msg );
	return;
    }

    IOPar iop;
    elpropsel.fillPar( iop );
    stratsynth_.setElasticProperties( iop );
}


void uiSynthParsGrp::newCB( CallBacker* )
{
    if ( !confirmSave() )
	return;

    const SynthGenParams defsgd = SynthGenParams();
    TypeSet<SynthID> ids;
    stratsynth_.getIDs( ids, StratSynth::DataMgr::NoProps );
    NotifyStopper nsrm( stratsynth_.entryRemoved );

    if ( ids.isEmpty() )
    { // Should not happen, just in case
	stratsynth_.synthChange();
	NotifyStopper nsadd( stratsynth_.entryAdded );
	synthSelected.trigger( stratsynth_.addSynthetic(defsgd) );
	return;
    }

    for ( int idx=ids.size()-1; idx>0; idx-- )
	stratsynth_.removeSynthetic( ids[idx] );

    const SynthID handledid = ids[0];
    NotifyStopper nsupd( stratsynth_.entryChanged );
    const bool res = stratsynth_.updateSynthetic( handledid, defsgd );
    initGrp( nullptr );
    if ( res )
	synthSelected.trigger( handledid );
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
    NotifyStopper nselprop( stratsynth_.elPropSelChanged );
    NotifyStopper nsadd( stratsynth_.entryAdded );
    stratsynth_.synthChange();
    if ( usePar(par) )
	lastsavedfnm_.set( dlg.ioObj()->mainFileName() );
    else
    { //Try to restore the previous state
	stratsynth_.synthChange();
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


void uiSynthParsGrp::expSynthCB( CallBacker* )
{
    uiStratSynthExport dlg( this, stratsynth_ );
    dlg.go();
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
    const SynthGenParams::SynthType synthtype =
		SynthGenParams::parseEnumSynthType( synthselgrp_->getType() );
    SynthGenParams sgp( synthtype );
    forwardInputNames( &sgp );
    parsChangedCB( cb );
}


void uiSynthParsGrp::parsChangedCB( CallBacker* )
{
    updatefld_->setSensitive( !synthnmlb_->isEmpty() );
    addnewfld_->setSensitive( true );
    parschanged_ = true;
}


void uiSynthParsGrp::nameChangedCB( CallBacker* )
{
    updatefld_->setSensitive( !synthnmlb_->isEmpty() );
    addnewfld_->setSensitive( true );
    namechanged_ = true;
}


void uiSynthParsGrp::putToScreen()
{
    NotifyStopper nssel( synthselgrp_->selectionChanged );
    NotifyStopper nspars( synthselgrp_->parsChanged );

    const BufferString synthnm( synthnmlb_->getText() );
    const SynthGenParams* cursgp = stratsynth_.getGenParams(
					    stratsynth_.find( synthnm ) );
    if ( cursgp )
	synthselgrp_->setFrom( *cursgp );
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

	synthselgrp_->setFrom( sgp );
    }
}


bool uiSynthParsGrp::getFromScreen( SynthGenParams& sgp )
{
    const uiRetVal uirv = synthselgrp_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv,return false);

    if ( !synthselgrp_->getGenParams(sgp) )
	return false;

    if ( sgp.needsInput() )
    {
	if ( !stratsynth_.find(sgp.inpsynthnm_).isValid() )
	    mErrRet(tr("Problem with Input synthetic data"),return false);
    }

    return true;
}



//uiSynthGenDlg

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth::DataMgr& gp )
    : uiDialog(p,uiDialog::Setup(tr("Specify Synthetic Parameters"),
				mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
				.modal(false))
{
    setCtrlStyle( CloseOnly );

    uisynthparsgrp_ = new uiSynthParsGrp( this, gp );
}


uiSynthGenDlg::~uiSynthGenDlg()
{
}
