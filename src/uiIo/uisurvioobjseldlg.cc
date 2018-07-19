/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

#include "uisurvioobjseldlg.h"

#include "uisurveyselect.h"
#include "uilistbox.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "filepath.h"
#include "uimsg.h"


uiSurvIOObjSelDlg::uiSurvIOObjSelDlg( uiParent* p, const IOObjContext& ctxt,
				      bool selmulti )
    : uiDialog(p,Setup(tr("Get %1 from any survey")
			.arg(ctxt.objectTypeName()),
			mNoDlgTitle,mODHelpKey(mSelObjFromOtherSurveyHelpID)))
    , ctxt_(*new IOObjContext(ctxt))
    , ismultisel_(selmulti)
{
    survsel_ = new uiSurveySelect( this );

    uiListBox::Setup lbsu( ismultisel_ ? OD::ChooseAtLeastOne
				       : OD::ChooseOnlyOne,
			   toUiString(ctxt.objectTypeName()) );
    objfld_ = new uiListBox( this, lbsu );
    objfld_->setHSzPol( uiObject::WideVar );
    objfld_->setStretch( 2, 2 );
    objfld_->attach( alignedBelow, survsel_ );

    mAttachCB( postFinalise(), uiSurvIOObjSelDlg::initWin );
    if ( !ismultisel_ )
	mAttachCB( objfld_->doubleClicked, uiSurvIOObjSelDlg::accept );
}


uiSurvIOObjSelDlg::~uiSurvIOObjSelDlg()
{
    deepErase( ioobjs_ );
}


void uiSurvIOObjSelDlg::initWin( CallBacker* )
{
    updWin( true );

    mAttachCB( survsel_->survDirChg, uiSurvIOObjSelDlg::survSelCB );
}


void uiSurvIOObjSelDlg::survSelCB( CallBacker* )
{
    updWin( false );
}


void uiSurvIOObjSelDlg::setSelected( const DBKey& dbky )
{
    seldbkys_.setEmpty();
    seldbkys_.add( dbky );
    if ( finalised() )
	updWin( true );
}


void uiSurvIOObjSelDlg::setSelected( const DBKeySet& dbkys )
{
    seldbkys_ = dbkys;
    if ( finalised() )
	updWin( true );
}


SurveyDiskLocation uiSurvIOObjSelDlg::surveyDiskLocation() const
{
    return survsel_->surveyDiskLocation();
}


void uiSurvIOObjSelDlg::updWin( bool withsurvsel )
{
    if ( withsurvsel )
	selSurvFromSelection();
    updateObjs();
    setSelection();
}


void uiSurvIOObjSelDlg::updateObjs()
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


void uiSurvIOObjSelDlg::selSurvFromSelection()
{
    if ( seldbkys_.isEmpty() )
	return;

    const DBKey& firstdbky = seldbkys_.first();
    const SurveyDiskLocation& sdl = firstdbky.surveyDiskLocation();
    if ( sdl != surveyDiskLocation() )
	survsel_->setSurveyDirName( sdl.dirname_ );
}


void uiSurvIOObjSelDlg::setSelection()
{
    objfld_->chooseAll( false );
    if ( seldbkys_.isEmpty() )
	return;

    const SurveyDiskLocation sdl = surveyDiskLocation();
    for ( const auto dbky : seldbkys_ )
    {
	if ( dbky->surveyDiskLocation() != sdl )
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


const IOObj* uiSurvIOObjSelDlg::ioObj( int idx ) const
{
    return ioobjs_.validIdx(idx) ? ioobjs_[idx] : 0;
}


FullDBKey uiSurvIOObjSelDlg::key( int idx ) const
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


BufferString uiSurvIOObjSelDlg::mainFileName( int idx ) const
{
    const IOObj* ioobj = ioObj( idx );
    return BufferString( ioobj ? ioobj->mainFileName() : "" );
}


bool uiSurvIOObjSelDlg::acceptOK()
{
    objfld_->getChosen( chosenidxs_ );
    return !chosenidxs_.isEmpty();
}
