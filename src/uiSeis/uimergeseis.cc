/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "seisioobjinfo.h"
#include "seismerge.h"
#include "ctxtioobj.h"
#include "genc.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "bufstringset.h"
#include "zdomain.h"

#include "uibatchjobdispatchersel.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

const char* uiMergeSeis::mergeSeisProgName()
{
    mDeclStaticString( ret );
    ret = GetODApplicationName( "od_merge_seis" );
    return ret.str();
}

uiMergeSeis::uiMergeSeis( uiParent* p,
				    const TypeSet<MultiID>* selids )
    : uiDialog(p,Setup(tr("Merge 3D Seismic Volumes"),mNoDlgTitle,
		       mODHelpKey(mMergeSeisHelpID)))
{
    auto* listsgrp = new uiGroup( this, "Lists group" );

    uiListBox::Setup avsu( OD::ChooseAtLeastOne, tr("Available Cubes"),
			   uiListBox::AboveMid );
    availablefld_ = new uiListBox( listsgrp, avsu, "available cubes" );
    availablefld_->setHSzPol( uiObject::Wide );
    availablefilter_ = new uiListBoxFilter( *availablefld_ );
    fillAvailableCubes( selids );

    auto* selectgrp = new uiButtonGroup( listsgrp, "Select cubes",
					 OD::Vertical );
    addbut_ = new uiToolButton( selectgrp, "rightarrow", uiStrings::sAdd(),
				mCB(this,uiMergeSeis,addCubeCB) );
    removebut_ = new uiToolButton( selectgrp, "leftarrow",
				   uiStrings::sRemove(),
				   mCB(this,uiMergeSeis,removeCubeCB) );
    selectgrp->attach( centeredRightOf, availablefld_ );

    uiListBox::Setup selsu( OD::ChooseOnlyOne, tr("Selected Cubes"),
			    uiListBox::AboveMid );
    selectedfld_ = new uiListBox( listsgrp, selsu, "selected cubes" );
    selectedfld_->setHSzPol( uiObject::Wide );
    selectedfld_->attach( rightTo, availablefld_ );
    selectedfld_->attach( ensureRightOf, selectgrp );

    auto* ordergrp = new uiButtonGroup( listsgrp, "Order cubes", OD::Vertical );
    moveupbut_ = new uiToolButton( ordergrp, "uparrow", uiStrings::sMoveUp(),
				   mCB(this,uiMergeSeis,moveCubeCB) );
    movedownbut_ = new uiToolButton( ordergrp, "downarrow",
				     uiStrings::sMoveDown(),
				     mCB(this,uiMergeSeis,moveCubeCB) );
    ordergrp->attach( centeredRightOf, selectedfld_ );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, listsgrp );

    stackfld_ = new uiGenInput( this, tr("Duplicate traces"),
				BoolInpSpec(true,tr("Stack"),tr("Use first")) );
    stackfld_->attach( ensureBelow, sep );

    uiSeisTransfer::Setup stsu( Seis::Vol );
    stsu.withnullfill( false ).fornewentry( true ).withstep( false );
    transffld_ = new uiSeisTransfer( this, stsu );
    transffld_->attach( alignedBelow, stackfld_ );

    IOObjContext ctxt( uiSeisSel::ioContext(Seis::Vol,false) );
    outfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, transffld_ );

    Batch::JobSpec js( mergeSeisProgName() );
    js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, outfld_ );

    mAttachCB( availablefld_->selectionChanged,
	       uiMergeSeis::selectionChangedCB );
    mAttachCB( availablefld_->itemChosen,
	       uiMergeSeis::selectionChangedCB );
    mAttachCB( availablefld_->doubleClicked,
	       uiMergeSeis::addCubeCB );
    mAttachCB( selectedfld_->selectionChanged,
	       uiMergeSeis::selectionChangedCB );
    mAttachCB( selectedfld_->doubleClicked,
	       uiMergeSeis::removeCubeCB );
    updateButtonSensitivity();
}


uiMergeSeis::~uiMergeSeis()
{
    detachAllNotifiers();
}


void uiMergeSeis::fillAvailableCubes( const TypeSet<MultiID>* selids )
{
    availableids_.erase();
    BufferStringSet nms;
    const IOObjContext ctxt( uiSeisSel::ioContext(Seis::Vol,true) );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    TypeSet<int> selindices;
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj || !ioobj->isUserSelectable() )
	    continue;

	const MultiID& key = ioobj->key();
	if ( selids && selids->isPresent(key) )
	    selindices.add( nms.size() );

	availableids_ += key;
	nms.add( ioobj->name() );
    }

    availablefilter_->setItems( nms );
    availablefld_->setNrLines( nms.size() < 10 ? 10 : 15 );
    if ( !selindices.isEmpty() )
	availablefld_->setChosen( selindices );
}


void uiMergeSeis::insertAvailable( const MultiID& mid )
{
    if ( availableids_.isPresent(mid) )
	return;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    const BufferString nm( ioobj ? ioobj->name().buf() : "" );
    int insidx = availableids_.size();
    for ( int idx=0; idx<availableids_.size(); idx++ )
    {
	PtrMan<IOObj> availobj = IOM().get( availableids_[idx] );
	if ( availobj && nm < availobj->name() )
	{
	    insidx = idx;
	    break;
	}
    }

    availableids_.insert( insidx, mid );
}


void uiMergeSeis::refreshAvailableList()
{
    BufferStringSet nms;
    for ( const auto& mid : availableids_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	nms.add( ioobj ? ioobj->name() : toString(mid) );
    }

    availablefilter_->setItems( nms );
}


void uiMergeSeis::addCubeCB( CallBacker* )
{
    TypeSet<int> idxs;
    availablefilter_->getChosen( idxs );
    TypeSet<MultiID> tomove;
    for ( const int idx : idxs )
    {
	if ( availableids_.validIdx(idx) )
	    tomove.addIfNew( availableids_[idx] );
    }

    if ( tomove.isEmpty() )
	return;

    for ( const auto& mid : tomove )
    {
	selectedids_ += mid;
	availableids_ -= mid;
    }

    refreshAvailableList();
    updateSelectedList( selectedids_.size()-1 );
    updateTransfld();
}


void uiMergeSeis::removeCubeCB( CallBacker* )
{
    const int selidx = selectedfld_->currentItem();
    if ( !selectedids_.validIdx(selidx) )
	return;

    const MultiID removedmid = selectedids_[selidx];
    selectedids_.removeSingle( selidx );
    insertAvailable( removedmid );
    refreshAvailableList();
    updateSelectedList( selidx );

    PtrMan<IOObj> ioobj = IOM().get( removedmid );
    if ( ioobj )
	availablefld_->setCurrentItem( ioobj->name().buf() );

    updateTransfld();
}


void uiMergeSeis::moveCubeCB( CallBacker* cb )
{
    const int fromidx = selectedfld_->currentItem();
    const int toidx = cb == movedownbut_ ? fromidx + 1 : fromidx - 1;
    if ( !selectedids_.validIdx(fromidx) || !selectedids_.validIdx(toidx) )
	return;

    selectedids_.swap( fromidx, toidx );
    updateSelectedList( toidx );
}


void uiMergeSeis::selectionChangedCB( CallBacker* )
{
    updateButtonSensitivity();
}


void uiMergeSeis::updateSelectedList( int curidx )
{
    selectedfld_->setEmpty();
    for ( const auto& mid : selectedids_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( ioobj )
	    selectedfld_->addItem( ioobj->uiName() );
    }

    if ( selectedfld_->isEmpty() )
	curidx = -1;
    else if ( curidx < 0 )
	curidx = 0;
    else if ( curidx >= selectedfld_->size() )
	curidx = selectedfld_->size() - 1;

    selectedfld_->setCurrentItem( curidx );
    updateButtonSensitivity();
}


void uiMergeSeis::updateButtonSensitivity()
{
    addbut_->setSensitive( availablefilter_->nrChosen() > 0 );

    const int selidx = selectedfld_->currentItem();
    const bool haveselection = selectedids_.validIdx( selidx );
    removebut_->setSensitive( haveselection );
    moveupbut_->setSensitive( haveselection && selidx > 0 );
    movedownbut_->setSensitive( haveselection &&
				selidx < selectedids_.size()-1 );
}


void uiMergeSeis::updateTransfld()
{
    TrcKeyZSampling tkzs;
    for ( const auto& mid : selectedids_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    continue;

	const SeisIOObjInfo sobj( ioobj.ptr() );
	TrcKeyZSampling inptkzs;
	sobj.getRanges( inptkzs );
	tkzs.include( inptkzs );
    }

    transffld_->setInput( tkzs );
}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    ObjectSet<IOPar> inpars;
    IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
    {
	deepErase( inpars );
	return false;
    }

    if ( batchfld_->wantBatch() )
    {
	const BufferString jobname( "Merge_", outioobj->name() );
	batchfld_->setJobName( jobname );
	IOPar& jobpars = batchfld_->jobSpec().pars_;
	jobpars.setEmpty();
	jobpars.set( "Task", "Merge" );
	jobpars.setYN( "Stack", stackfld_->getBoolValue() );

	IOPar allinpars;
	for ( int idx=0; idx<inpars.size(); idx++ )
	    allinpars.mergeComp( *inpars[idx], toString(idx) );
	jobpars.mergeComp( allinpars, sKey::Input() );
	jobpars.mergeComp( outpar, sKey::Output() );
	deepErase( inpars );

	batchfld_->saveProcPars( *outioobj );
	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    SeisMerger mrgr( inpars, outpar, false );
    deepErase( inpars );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    mrgr.setScaler( transffld_->getScaler() );

    uiTaskRunner dlg( this );
    const bool success = TaskRunner::execute( &dlg, mrgr );
    if ( !success )
    {
	uiMSG().errorWithDetails( dlg.errorWithDetails() );
	return false;
    }

    const uiString msg = tr("Seismic Data merge was successful. Merge more?");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
					   tr("No, close window") );
    return !ret;
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    outfld_->reset();
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    if ( selectedids_.size() < 2 )
    {
	uiMSG().error( tr("Please select at least 2 inputs") );
	return false;
    }

    outpar.set( sKey::ID(), outioobj->key() );
    transffld_->fillPar( outpar );

    BufferString typestr, zdomstr;
    for ( int idx=0; idx<selectedids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selectedids_[idx] );
	if ( !ioobj )
	    continue;

	const BufferString curtypestr( ioobj->pars().find(sKey::Type()) );
	const BufferString curzdomstr( ioobj->pars().find(ZDomain::sKey()) );
	if ( !idx )
	{
	    typestr = curtypestr;
	    zdomstr = curzdomstr;
	}
	else if ( !SeisIOObjInfo::isCompatibleType(typestr,curtypestr) )
	{
	    uiMSG().error( tr("Input cubes are of incompatible types") );
	    return false;
	}
	else if ( &ZDomain::Def::get(zdomstr)
		  != &ZDomain::Def::get(curzdomstr) )
	{
	    uiMSG().error( tr("Input cubes should belong to the same"
			      " Z domain") );
	    return false;
	}

	auto* inpar = new IOPar;
	inpar->set( sKey::ID(), selectedids_[idx] );
	transffld_->fillPar( *inpar );
	inpars += inpar;
    }

    uiSeisIOObjInfo ioobjinfo( *outioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    if ( typestr.isEmpty() && zdomstr.isEmpty() )
	return true;

    outioobj->pars().update( sKey::Type(), typestr );
    outioobj->pars().update( ZDomain::sKey(), zdomstr );
    IOM().commitChanges( *outioobj );

    return true;
}
