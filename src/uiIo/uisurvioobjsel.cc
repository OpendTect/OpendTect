/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uisurvioobjseldlg.h"
#include "uisurvioobjselgrp.h"

#include "uisurveyselect.h"
#include "uilistbox.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "filepath.h"
#include "uimsg.h"


uiSurvIOObjSelGroup::uiSurvIOObjSelGroup( uiParent* p, const IOObjContext& ctxt,
					  bool selmulti )
    : uiGroup(p,"Survey IOObj Selector")
    , ctxt_(*new IOObjContext(ctxt))
    , ismultisel_(selmulti)
    , dClicked(this)
    , survChange(this)
    , selChange(this)
{
    survsel_ = new uiSurveySelect( this );

    uiListBox::Setup lbsu( ismultisel_ ? OD::ChooseAtLeastOne
				       : OD::ChooseOnlyOne,
			   toUiString(ctxt.objectTypeName()) );
    objfld_ = new uiListBox( this, lbsu );
    objfld_->setHSzPol( uiObject::WideVar );
    objfld_->setStretch( 2, 2 );
    objfld_->attach( alignedBelow, survsel_ );
    mAttachCB( objfld_->selectionChanged, uiSurvIOObjSelGroup::selChgCB );

    mAttachCB( postFinalise(), uiSurvIOObjSelGroup::initGrp );
    if ( !ismultisel_ )
	mAttachCB( objfld_->doubleClicked, uiSurvIOObjSelGroup::dClickCB );
}


uiSurvIOObjSelGroup::~uiSurvIOObjSelGroup()
{
    deepErase( ioobjs_ );
}


void uiSurvIOObjSelGroup::initGrp( CallBacker* )
{
    updGrp( true );

    mAttachCB( survsel_->survDirChg, uiSurvIOObjSelGroup::survSelCB );
}


void uiSurvIOObjSelGroup::dClickCB( CallBacker* )
{
    dClicked.trigger();
}


void uiSurvIOObjSelGroup::selChgCB( CallBacker* )
{
    selChange.trigger();
}


void uiSurvIOObjSelGroup::survSelCB( CallBacker* )
{
    updGrp( false );
    survChange.trigger();
}


void uiSurvIOObjSelGroup::setSurvey( const SurveyDiskLocation& sdl )
{
    if ( sdl == surveyDiskLocation() )
	return;

    seldbkys_.setEmpty();
    survsel_->setSurveyDiskLocation( sdl );
    if ( finalised() )
	updGrp( false );
}


void uiSurvIOObjSelGroup::setSelected( const DBKey& dbky )
{
    seldbkys_.setEmpty();
    seldbkys_.add( dbky );
    if ( finalised() )
	updGrp( true );
}


void uiSurvIOObjSelGroup::setSelected( const DBKeySet& dbkys )
{
    seldbkys_ = dbkys;
    if ( finalised() )
	updGrp( true );
}


void uiSurvIOObjSelGroup::addExclude( const SurveyDiskLocation& sdl )
{
    survsel_->addExclude( sdl );
}


SurveyDiskLocation uiSurvIOObjSelGroup::surveyDiskLocation() const
{
    return survsel_->surveyDiskLocation();
}


void uiSurvIOObjSelGroup::updGrp( bool withsurvsel )
{
    if ( withsurvsel )
	selSurvFromSelection();
    updateObjs();
    setSelection();
}


void uiSurvIOObjSelGroup::updateObjs()
{
    deepErase( ioobjs_ );
    objfld_->setEmpty();

    const BufferString datadirnm( surveyDiskLocation().fullPathFor(
			    ctxt_.getDataDirName(ctxt_.stdseltype_,true) ) );
    ConstRefMan<DBDir> dbdir = new DBDir( datadirnm );
    BufferStringSet objnms;
    DBDirIter iter( *dbdir );
    ObjectSet<IOObj> objs;
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ctxt_.validIOObj(ioobj) )
	{
	    IOObj* toadd = ioobj.clone();
	    toadd->setAbsDirectory( datadirnm );
	    objs += toadd;
	    objnms.add( toadd->name() );
	}
    }
    if ( objnms.isEmpty() )
	return;

    BufferStringSet::size_type* idxs = objnms.getSortIndexes();
    objnms.useIndexes( idxs );
    for ( int idx=0; idx<objs.size(); idx++ )
	ioobjs_ += objs[ idxs[idx] ];
    delete [] idxs;

    objfld_->addItems( objnms );
    objfld_->setCurrentItem( 0 );
}


void uiSurvIOObjSelGroup::selSurvFromSelection()
{
    if ( seldbkys_.isEmpty() )
	return;

    mDynamicCastGet( const FullDBKey*, fdbky, &seldbkys_.first() );
    if ( fdbky && fdbky->surveyDiskLocation() != surveyDiskLocation() )
	survsel_->setSurveyDiskLocation( fdbky->surveyDiskLocation() );
}


void uiSurvIOObjSelGroup::setSelection()
{
    objfld_->chooseAll( false );
    if ( seldbkys_.isEmpty() )
	return;

    const SurveyDiskLocation sdl = surveyDiskLocation();
    for ( const auto dbky : seldbkys_ )
    {
	mDynamicCastGet( FullDBKey*, fdbky, dbky )
	if ( fdbky && fdbky->surveyDiskLocation() != sdl )
	    continue;

	for ( int idx=0; idx<ioobjs_.size(); idx++ )
	{
	    if ( ioobjs_[idx]->key() == *dbky )
	    {
		objfld_->setChosen( idx, true );
		if ( !ismultisel_ )
		    return;
	    }
	}
    }
}


const IOObj* uiSurvIOObjSelGroup::ioObj( int idx ) const
{
    const int ioobjidx = chosenidxs_.validIdx(idx) ? chosenidxs_[idx] : -1;
    return ioobjs_.validIdx(ioobjidx) ? ioobjs_[ioobjidx] : 0;
}


FullDBKey uiSurvIOObjSelGroup::key( int idx ) const
{
    FullDBKey ret( surveyDiskLocation() );
    if ( !chosenidxs_.validIdx(idx) )
	return ret;

    const int ioobjidx = chosenidxs_[idx];
    if ( !ioobjs_.validIdx(ioobjidx) )
	{ pErrMsg("Bad idx"); return ret; }

    ret.setKey( ioobjs_[ioobjidx]->key() );
    return ret;
}


BufferString uiSurvIOObjSelGroup::mainFileName( int idx ) const
{
    const IOObj* ioobj = ioObj( idx );
    return BufferString( ioobj ? ioobj->mainFileName() : "" );
}


bool uiSurvIOObjSelGroup::evaluateInput()
{
    objfld_->getChosen( chosenidxs_ );
    return !chosenidxs_.isEmpty();
}


uiSurvIOObjSelDlg::uiSurvIOObjSelDlg( uiParent* p, const IOObjContext& ctxt,
				      bool selmulti )
    : uiDialog(p,Setup(tr("Get %1 from another survey")
			.arg(ctxt.objectTypeName()),
			mNoDlgTitle,mODHelpKey(mSelObjFromOtherSurveyHelpID)))
{
    selgrp_ = new uiSurvIOObjSelGroup( this, ctxt, selmulti );
    mAttachCB( selgrp_->dClicked, uiSurvIOObjSelDlg::accept );
}


void uiSurvIOObjSelDlg::setSurvey( const SurveyDiskLocation& sdl )
{
    selgrp_->setSurvey( sdl );
}


void uiSurvIOObjSelDlg::setSelected( const DBKey& dbky )
{
    selgrp_->setSelected( dbky );
}


void uiSurvIOObjSelDlg::setSelected( const DBKeySet& dbkys )
{
    selgrp_->setSelected( dbkys );
}


int uiSurvIOObjSelDlg::nrSelected() const
{
    return selgrp_->nrSelected();
}


void uiSurvIOObjSelDlg::addExclude( const SurveyDiskLocation& sdl )
{
    selgrp_->addExclude( sdl );
}


SurveyDiskLocation uiSurvIOObjSelDlg::surveyDiskLocation() const
{
    return selgrp_->surveyDiskLocation();
}


const IOObj* uiSurvIOObjSelDlg::ioObj( int idx ) const
{
    return selgrp_->ioObj( idx );
}


FullDBKey uiSurvIOObjSelDlg::key( int idx ) const
{
    return selgrp_->key( idx );
}


BufferString uiSurvIOObjSelDlg::mainFileName( int idx ) const
{
    return selgrp_->mainFileName( idx );
}


const ObjectSet<IOObj>& uiSurvIOObjSelDlg::objsInSurvey() const
{
    return selgrp_->objsInSurvey();
}


bool uiSurvIOObjSelDlg::acceptOK()
{
    return selgrp_->evaluateInput();
}
