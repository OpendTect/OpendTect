/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uisurvioobjsel.h"
#include "uisurvioobjseldlg.h"
#include "uisurvioobjselgrp.h"

#include "uisurveyselect.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uibutton.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "filepath.h"
#include "uimsg.h"


static void getIOObList( ObjectSet<IOObj>& objs, const SurveyDiskLocation& sdl,
			 const IOObjContext& ctxt, BufferStringSet& nms )
{
    deepErase( objs );
    nms.setEmpty();

    const BufferString datadirnm( sdl.fullPathFor(
			    ctxt.getDataDirName(ctxt.stdseltype_,true) ) );
    ConstRefMan<DBDir> dbdir = new DBDir( datadirnm );
    DBDirIter iter( *dbdir );
    ObjectSet<IOObj> unsortedobjs;
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ctxt.validIOObj(ioobj) )
	{
	    IOObj* toadd = ioobj.clone();
	    toadd->setAbsDirectory( datadirnm );
	    unsortedobjs += toadd;
	    nms.add( toadd->name() );
	}
    }
    if ( nms.isEmpty() )
	return;

    BufferStringSet::size_type* idxs = nms.getSortIndexes();
    nms.useIndexes( idxs );
    for ( int idx=0; idx<unsortedobjs.size(); idx++ )
	objs += unsortedobjs[ idxs[idx] ];
    delete [] idxs;
}


uiSurvIOObjSelGroup::uiSurvIOObjSelGroup( uiParent* p, const IOObjContext& ctxt,
					  bool selmulti, bool fixsurv )
    : uiGroup(p,"Survey IOObj Sel Group")
    , ctxt_(*new IOObjContext(ctxt))
    , survsel_(0)
    , ismultisel_(selmulti)
    , dClicked(this)
    , survChange(this)
    , selChange(this)
{
    if ( !fixsurv )
	survsel_ = new uiSurveySelect( this );

    uiListBox::Setup lbsu( ismultisel_ ? OD::ChooseAtLeastOne
				       : OD::ChooseOnlyOne,
			   ctxt.uiObjectTypeName(ismultisel_ ? mPlural : 1) );
    objfld_ = new uiListBox( this, lbsu );
    objfld_->setHSzPol( uiObject::WideVar );
    objfld_->setStretch( 2, 2 );
    if ( survsel_ )
	objfld_->attach( alignedBelow, survsel_ );

    setHAlignObj( objfld_ );
    mAttachCB( postFinalise(), uiSurvIOObjSelGroup::initGrp );
}


uiSurvIOObjSelGroup::~uiSurvIOObjSelGroup()
{
    deepErase( ioobjs_ );
    delete &ctxt_;
}


void uiSurvIOObjSelGroup::initGrp( CallBacker* )
{
    updGrp( true );

    mAttachCB( objfld_->selectionChanged, uiSurvIOObjSelGroup::selChgCB );
    if ( !ismultisel_ )
	mAttachCB( objfld_->doubleClicked, uiSurvIOObjSelGroup::dClickCB );
    if ( survsel_ )
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

    survloc_ = sdl;
    seldbkys_.setEmpty();
    if ( survsel_ )
	survsel_->setSurveyDiskLocation( sdl );

    if ( finalised() )
	updGrp( false );
}


int uiSurvIOObjSelGroup::indexOf( const DBKey& dbky )
{
    mDynamicCastGet( const FullDBKey*, fdbky, &dbky )
    for ( int idx=0; idx<ioobjs_.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs_[idx];
	if ( (fdbky && ioobj.fullKey() == *fdbky)
	  || (!fdbky && ioobj.key() == dbky) )
	    return idx;
    }
    return -1;
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
    if ( survsel_ )
	survsel_->addExclude( sdl );
}


SurveyDiskLocation uiSurvIOObjSelGroup::surveyDiskLocation() const
{
    return survsel_ ? survsel_->surveyDiskLocation() : survloc_;
}


void uiSurvIOObjSelGroup::setSurveySelectable( bool yn )
{
    if ( survsel_ )
	survsel_->setSensitive( yn );
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
    BufferStringSet nms;
    getIOObList( ioobjs_, surveyDiskLocation(), ctxt_, nms );
    objfld_->setEmpty();
    objfld_->addItems( nms );
    objfld_->setCurrentItem( 0 );
}


void uiSurvIOObjSelGroup::selSurvFromSelection()
{
    if ( !survsel_ || seldbkys_.isEmpty() )
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


DBKey uiSurvIOObjSelGroup::key( int idx ) const
{
    if ( chosenidxs_.validIdx(idx) )
    {
	const int ioobjidx = chosenidxs_[idx];
	if ( !ioobjs_.validIdx(ioobjidx) )
	    { pErrMsg("Bad idx"); }
	else
	    return ioobjs_[ioobjidx]->key();
    }
    return DBKey();
}


FullDBKey uiSurvIOObjSelGroup::fullKey( int idx ) const
{
    return FullDBKey( surveyDiskLocation(), key(idx) );
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
				      bool selmulti, bool fixsurv )
    : uiDialog(p,Setup(uiString::empty(),
			mNoDlgTitle,mODHelpKey(mSelObjFromOtherSurveyHelpID)))
{
    selgrp_ = new uiSurvIOObjSelGroup( this, ctxt, selmulti, fixsurv );
    mAttachCB( postFinalise(), uiSurvIOObjSelDlg::initWin );
}


void uiSurvIOObjSelDlg::initWin( CallBacker* )
{
    survChgCB( 0 );
    mAttachCB( selgrp_->dClicked, uiSurvIOObjSelDlg::accept );
    mAttachCB( selgrp_->survChange, uiSurvIOObjSelDlg::survChgCB );
}


void uiSurvIOObjSelDlg::survChgCB( CallBacker* )
{
    setCaption( uiStrings::phrSelect( toUiString("%1 @ '%2'")
		.arg( selgrp_->ioobjContext().uiObjectTypeName() )
		.arg( surveyDiskLocation().surveyName() ) ) );
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


void uiSurvIOObjSelDlg::setSurveySelectable( bool yn )
{
    selgrp_->setSurveySelectable( yn );
}


const IOObj* uiSurvIOObjSelDlg::ioObj( int idx ) const
{
    return selgrp_->ioObj( idx );
}


FullDBKey uiSurvIOObjSelDlg::fullKey( int idx ) const
{
    return selgrp_->fullKey( idx );
}


DBKey uiSurvIOObjSelDlg::key( int idx ) const
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


uiSurvIOObjSel::uiSurvIOObjSel( uiParent* p, const IOObjContext& ctxt,
				const uiString& lbltxt, bool surveyfixed )
    : uiGroup(p,"Survey IOObj Selector")
    , ctxt_(*new IOObjContext(ctxt))
    , selChange(this)
    , survChange(this)
{
    objfld_ = new uiComboBox( this, "Objects" );

    if ( !lbltxt.isEmpty() )
	lbl_ = new uiLabel( this, lbltxt, objfld_ );

    if ( !surveyfixed )
    {
	survselbut_ = uiButton::getStd( this, OD::Select,
		mCB(this,uiSurvIOObjSel,selSurvCB), true, uiString::empty() );
	survselbut_->attach( rightOf, objfld_ );
    }

    setHAlignObj( objfld_ );
    mAttachCB( postFinalise(), uiSurvIOObjSel::initGrp );
}


uiSurvIOObjSel::~uiSurvIOObjSel()
{
    deepErase( ioobjs_ );
    delete &ctxt_;
}


void uiSurvIOObjSel::initGrp( CallBacker* )
{
    updateObjs();
    updateUi();

    mAttachCB( objfld_->selectionChanged, uiSurvIOObjSel::selChgCB );
}


void uiSurvIOObjSel::selChgCB( CallBacker* )
{
    selChange.trigger();
}


void uiSurvIOObjSel::selSurvCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(
				uiStrings::phrSelect(ctxt_.uiObjectTypeName()),
				mNoDlgTitle,mTODOHelpKey) );
    uiSurveySelect* survsel = new uiSurveySelect( &dlg );
    for ( auto sdl : excludes_ )
	survsel->addExclude( sdl );
    survsel->setSurveyDiskLocation( survloc_ );

    if ( dlg.go() )
	setSurvey( survsel->surveyDiskLocation() );
}


void uiSurvIOObjSel::setSurvey( const SurveyDiskLocation& sdl )
{
    if ( sdl == survloc_ )
	return;

    survloc_ = sdl;
    updateObjs();
    if ( finalised() )
    {
	updateUi();
	survChange.trigger();
    }
}


void uiSurvIOObjSel::setSelected( const FullDBKey& fdbky )
{
    const SurveyDiskLocation newsdl = fdbky.surveyDiskLocation();
    if ( survloc_ == newsdl )
	setSelected( DBKey(fdbky) );
    else
    {
	survloc_ = newsdl;
	updateObjs();
	updateUi( &fdbky );
    }
}


void uiSurvIOObjSel::setSelected( DBKey dbky )
{
    if ( !dbky.isValid() || dbky == key() )
	return;

    for ( int idx=0; idx<ioobjs_.size(); idx++ )
    {
	if ( ioobjs_[idx]->key() == dbky )
	{
	    objfld_->setCurrentItem( idx );
	    selChange.trigger();
	    break;
	}
    }
}


void uiSurvIOObjSel::setSelected( const char* nm, bool mostsimilar )
{
    int selidx = ioobjnames_.indexOf( nm );
    if ( selidx < 0 && mostsimilar )
	selidx = ioobjnames_.nearestMatch( nm );
    if ( selidx >= 0 )
	setSelected( ioobjs_[selidx]->key() );
}


void uiSurvIOObjSel::addExclude( const SurveyDiskLocation& sdl )
{
    excludes_ += sdl;
}


void uiSurvIOObjSel::setSurveySelectable( bool yn )
{
    if ( survselbut_ )
	survselbut_->setSensitive( yn );
}


void uiSurvIOObjSel::setLblText( const uiString& newlbltxt )
{
    if ( lbl_ )
	lbl_->setText( newlbltxt );
}


void uiSurvIOObjSel::updateObjs()
{
    getIOObList( ioobjs_, surveyDiskLocation(), ctxt_, ioobjnames_ );
}


void uiSurvIOObjSel::updateUi( const DBKey* dbky )
{
    objfld_->setEmpty();
    objfld_->addItems( ioobjnames_ );
    if ( ioobjnames_.isEmpty() )
	return;

    int selidx = 0;
    if ( dbky )
    {
	for ( int idx=0; idx<ioobjs_.size(); idx++ )
	    if ( ioobjs_[idx]->key() == *dbky )
		{ selidx = idx; break; }
    }
    objfld_->setCurrentItem( selidx );
}


const IOObj* uiSurvIOObjSel::ioObj() const
{
    const int ioobjidx = objfld_->currentItem();
    return ioobjs_.validIdx(ioobjidx) ? ioobjs_[ioobjidx] : 0;
}


DBKey uiSurvIOObjSel::key() const
{
    const IOObj* ioobj = ioObj();
    if ( ioobj )
	return ioobj->key();
    return DBKey();
}


FullDBKey uiSurvIOObjSel::fullKey() const
{
    return FullDBKey( survloc_, key() );
}


BufferString uiSurvIOObjSel::ioObjName() const
{
    const IOObj* ioobj = ioObj();
    return BufferString( ioobj ? ioobj->name() : "" );
}


BufferString uiSurvIOObjSel::mainFileName() const
{
    const IOObj* ioobj = ioObj();
    return BufferString( ioobj ? ioobj->mainFileName() : "" );
}
