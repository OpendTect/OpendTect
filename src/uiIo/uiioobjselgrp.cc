/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		25/05/2000
________________________________________________________________________

-*/

#include "uiioobjselgrp.h"
#include "uiioobjinserter.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "dbdir.h"
#include "file.h"
#include "keystrs.h"
#include "iopar.h"
#include "iostrm.h"
#include "ioobjselectiontransl.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "settings.h"
#include "sorting.h"
#include "od_helpids.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uilabel.h"

#define mObjTypeName ctio_.ctxt_.objectTypeName()
static const BufferString sKeyTimeSort(
	IOPar::compKey("dTect.Disp.Objects",sKey::TimeSort()) );

static const char* dGBToDispStorageStr()    { return "OpendTect";  }


class uiIOObjSelGrpManipSubj : public uiIOObjManipGroupSubj
{ mODTextTranslationClass(uiIOObjSelGrpManipSubj);
public:

uiIOObjSelGrpManipSubj( uiIOObjSelGrp* sg )
    : uiIOObjManipGroupSubj(sg->listfld_->box())
    , selgrp_(sg)
    , manipgrp_(0)
    , itemChanged(this)
{
    mAttachCB( selgrp_->selectionChanged, uiIOObjSelGrpManipSubj::selChg );
    mAttachCB( selgrp_->itemChosen, uiIOObjSelGrpManipSubj::selChg );
}


DBKey currentID() const
{
    return selgrp_->currentID();
}


void getChosenIDs( DBKeySet& mids ) const
{
    selgrp_->getChosen( mids );
}


void getChosenNames( BufferStringSet& nms ) const
{
    selgrp_->getChosen( nms );
}

const char* defExt() const
{
    return selgrp_->ctio_.ctxt_.trgroup_->defExtension();
}

const BufferStringSet& names() const
{
    return selgrp_->ioobjnms_;
}

void chgsOccurred()
{
    selgrp_->fullUpdate( selgrp_->listfld_->currentItem() );
    itemChanged.trigger();
}

void selChg( CallBacker* )
{
    if ( manipgrp_ ) manipgrp_->selChg();
}

void relocStart( const char* msg )
{
    selgrp_->triggerStatusMsg( msg );
}

    uiIOObjSelGrp*	selgrp_;
    uiIOObjManipGroup*	manipgrp_;

    Notifier<uiIOObjManipGroupSubj> itemChanged;

};


#define muiIOObjSelGrpConstructorCommons \
      uiGroup(p) \
    , ctio_(*new CtxtIOObj(c)) \
    , lbchoiceio_(0) \
    , newStatusMsg(this) \
    , selectionChanged(this) \
    , itemChosen(this) \
    , itemChanged(this)


// Note: don't combine first and second constructor making the uiString default
// that will make things compile that shouldn't


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c )
    : muiIOObjSelGrpConstructorCommons
{ init(); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
			      const uiString& seltxt )
    : muiIOObjSelGrpConstructorCommons
{ init( seltxt ); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
			      const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{ init(); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
		      const uiString& seltxt, const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{ init( seltxt ); }


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c )
    : muiIOObjSelGrpConstructorCommons
{ init(); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const uiString& seltxt )
    : muiIOObjSelGrpConstructorCommons
{ init( seltxt ); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{ init(); }
uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
		      const uiString& seltxt, const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{ init( seltxt ); }


void uiIOObjSelGrp::init( const uiString& seltxt )
{
    ctio_.ctxt_.fillTrGroup();
    nmfld_ = 0; wrtrselfld_ = 0;
    manipgrpsubj = 0; mkdefbut_ = 0; asked2overwrite_ = false;
    if ( !ctio_.ctxt_.forread_ )
	setup_.choicemode( OD::ChooseOnlyOne );

    mkTopFlds( seltxt );
    if ( !ctio_.ctxt_.forread_ )
	mkWriteFlds();
    mkManipulators();

    setHAlignObj( topgrp_ );
    mAttachCB( postFinalise(), uiIOObjSelGrp::setInitial );
}


void uiIOObjSelGrp::mkTopFlds( const uiString& seltxt )
{
    topgrp_ = new uiGroup( this, "Top group" );

    uiListBox::Setup su( setup_.choicemode_, seltxt );
    listfld_ = new uiListBox( topgrp_, su, "Objects" );

    filtfld_ = new uiLineEdit( listfld_, StringInpSpec("*"), "Filter" );
    filtfld_->setHSzPol( uiObject::SmallVar );
    filtfld_->setToolTip( uiStrings::sFilter() );
    mAttachCB( filtfld_->editingFinished, uiIOObjSelGrp::orderChgCB );
    mAttachCB( filtfld_->returnPressed, uiIOObjSelGrp::orderChgCB );

    if ( setup_.withctxtfilter_ )
    {
	const DBDirEntryList entrylist( ctio_.ctxt_ );
	BufferStringSet valstrs = entrylist.getParValuesFor(
						    setup_.withctxtfilter_ );

	const int idx = valstrs.indexOf( mDGBKey );
	if ( idx > -1 )
	    valstrs.get(idx).set( dGBToDispStorageStr() );
	//This is strictly for display purpose without changing the key
	if ( valstrs.size()>1 )
	{
	    valstrs.sort();
	    uiStringSet uistrset = valstrs.getUiStringSet();
	    uistrset.insert( 0, tr("All %1").arg( setup_.withctxtfilter_ ) );
	    ctxtfiltfld_ = new uiComboBox( listfld_, uistrset, "ctxtfilter" );
	    auto* lbl = new uiLabel( listfld_, uiStrings::sType() );
	    ctxtfiltfld_->attach( alignedBelow, filtfld_ );
	    lbl->attach( leftOf, ctxtfiltfld_ );
	    mAttachCB( ctxtfiltfld_->selectionChanged,
		       uiIOObjSelGrp::ctxtChgCB );
	}
    }

    didtsort_ = false;

    if ( setup_.withsort_ )
    {
	tsortbox_ = new uiCheckBox( listfld_, uiStrings::sTimeSort() );
	Settings::common().getYN( sKeyTimeSort, didtsort_ );
	tsortbox_->setChecked( didtsort_ );
	tsortbox_->setHSzPol( uiObject::SmallVar );
	mAttachCB( tsortbox_->activated, uiIOObjSelGrp::sortChgCB );
    }

    uiToolButton* refreshbut = nullptr;
    if ( setup_.withrefresh_ )
    {
	refreshbut = new uiToolButton( listfld_,
		"refresh", tr("Refresh"), mCB(this,uiIOObjSelGrp,refreshCB) );
	refreshbut->attach( rightBorder, listfld_->box() );
    }

    if ( tsortbox_ )
    {
	if ( refreshbut )
	    tsortbox_->attach( leftOf, refreshbut );
	else
	    tsortbox_->attach( rightAlignedAbove, listfld_->box() );
	filtfld_->attach( leftOf, tsortbox_ );
    }

    if ( ctxtfiltfld_ )
	listfld_->box()->attach( centeredBelow, ctxtfiltfld_ );
    else
	listfld_->box()->attach( centeredBelow, filtfld_ );

    topgrp_->setHAlignObj( listfld_ );

    listfld_->setName( "Objects list" );
    listfld_->box()->setPrefHeightInChar( 8 );
    listfld_->setHSzPol( uiObject::WideVar );
    if ( isMultiChoice() )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_, mObjTypeName );
	mAttachCB( lbchoiceio_->readDone, uiIOObjSelGrp::readChoiceDone );
	mAttachCB( lbchoiceio_->storeRequested, uiIOObjSelGrp::writeChoiceReq );
    }

    topgrp_->setHAlignObj( listfld_ );
    fullUpdate( -1 );
    if ( ctio_.ioobj_ )
	listfld_->setCurrentItem( ctio_.ioobj_->name() );
}


void uiIOObjSelGrp::mkWriteFlds()
{
    uiGroup* wrgrp = new uiGroup( this, "Write group" );
    wrtrselfld_ = 0;
    if ( setup_.withwriteopts_ )
    {
	wrtrselfld_ = new uiIOObjSelWriteTranslator( wrgrp, ctio_, true );
	mAttachCB( wrtrselfld_->suggestedNameAvailble, uiIOObjSelGrp::nameAvCB);
    }

    nmfld_ = new uiGenInput( wrgrp, uiStrings::sName() );
    nmfld_->setElemSzPol( uiObject::SmallMax );
    nmfld_->setStretch( 2, 0 );
    mAttachCB( nmfld_->valuechanged, uiIOObjSelGrp::newOutputNameCB );
    if ( wrtrselfld_ && !wrtrselfld_->isEmpty() )
	nmfld_->attach( alignedBelow, wrtrselfld_ );
    wrgrp->setHAlignObj( nmfld_ );
    wrgrp->attach( alignedBelow, topgrp_ );

    const BufferString nm( ctio_.name() );
    if ( !nm.isEmpty() )
    {
	nmfld_->setText( nm );
	const int listidx = listfld_->indexOf( nm );
	if ( listidx >= 0 )
	    listfld_->setCurrentItem( listidx );
    }
}


void uiIOObjSelGrp::mkManipulators()
{
    manipgrpsubj = new uiIOObjSelGrpManipSubj( this );
    manipgrpsubj->manipgrp_ = new uiIOObjManipGroup( *manipgrpsubj,
						     setup_.allowreloc_,
						     setup_.allowremove_ );
    if ( setup_.allowsetdefault_ )
    {
	mkdefbut_ = manipgrpsubj->manipgrp_->addButton(
	    "makedefault", uiStrings::phrSetAs(uiStrings::sDefault()),
	    mCB(this,uiIOObjSelGrp,makeDefaultCB) );
    }

    if ( !setup_.withinserters_ || uiIOObjInserter::allDisabled() )
	return;

    if ( !ctio_.ctxt_.forread_
      || !uiIOObjInserter::isPresent(*ctio_.ctxt_.trgroup_) )
	return;

    uiGroup* insbutgrp = new uiGroup( listfld_->parent(),
					"IOObj insert buttons" );
    const ObjectSet<const Translator>& tpls = ctio_.ctxt_.trgroup_->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
    {
	if ( !IOObjSelConstraints::isAllowedTranslator(tpls[idx]->userName(),
					ctio_.ctxt_.toselect_.allowtransls_) )
	    continue;

	uiIOObjInserter* inserter = uiIOObjInserter::create( *tpls[idx] );
	if ( !inserter || inserter->isDisabled() )
	    continue;
	uiToolButtonSetup* tbsu = inserter->getButtonSetup();
	if ( !tbsu )
	    { delete inserter; continue; }

	uiButton* but = tbsu->getButton( insbutgrp, true );
	delete tbsu;
	const int prevnrbuts = insertbuts_.size();
	insertbuts_ += but;
	if ( prevnrbuts > 0 )
	    but->attach( rightAlignedBelow, insertbuts_[prevnrbuts-1] );

	mAttachCB( inserter->objectInserted, uiIOObjSelGrp::objInserted );
	inserters_ += inserter;
    }
    insbutgrp->attach( centeredLeftOf, listfld_ );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    detachAllNotifiers();
    deepErase( inserters_ );
    if ( manipgrpsubj )
    {
	delete manipgrpsubj->manipgrp_;
	delete manipgrpsubj;
    }
    delete ctio_.ioobj_;
    delete &ctio_;
    delete lbchoiceio_;
}


bool uiIOObjSelGrp::isEmpty() const { return listfld_->isEmpty(); }
int uiIOObjSelGrp::size() const { return listfld_->size(); }
int uiIOObjSelGrp::currentItem() const { return listfld_->currentItem(); }
int uiIOObjSelGrp::nrChosen() const { return listfld_->nrChosen(); }
bool uiIOObjSelGrp::isChosen( int idx ) const { return listfld_->isChosen(idx);}
void uiIOObjSelGrp::setChosen( int idx, bool yn )
{ listfld_->setChosen(idx,yn);}
void uiIOObjSelGrp::chooseAll( bool yn ) { listfld_->chooseAll(yn);}


DBKey uiIOObjSelGrp::currentID() const
{
    const int selidx = listfld_->currentItem();
    return ioobjids_.validIdx(selidx) ? ioobjids_[selidx] : DBKey::getInvalid();
}


DBKey uiIOObjSelGrp::chosenID( int objnr ) const
{
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( isChosen(idx) )
	    objnr--;
	if ( objnr < 0 )
	    return ioobjids_[idx];
    }

    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrChosen()="; msg += nrChosen();
    pErrMsg( msg );
    return DBKey::getInvalid();
}


void uiIOObjSelGrp::getChosen( DBKeySet& mids ) const
{
    mids.setEmpty();
    const int nrchosen = nrChosen();
    for ( int idx=0; idx<nrchosen; idx++ )
	mids += chosenID( idx );
}


void uiIOObjSelGrp::getChosen( BufferStringSet& nms ) const
{
    nms.setEmpty();
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    nms.add( ioobjnms_.get( idx ) );
    }
}


void uiIOObjSelGrp::setCurrent( int curidx )
{
    if ( curidx < 0 )
	return;

    if ( curidx >= ioobjnms_.size() )
	curidx = ioobjnms_.size() - 1;

    listfld_->setCurrentItem( curidx );
    selectionChanged.trigger();
}


void uiIOObjSelGrp::setCurrent( const DBKey& dbky )
{
    setCurrent( ioobjids_.indexOf( dbky ) );
}


void uiIOObjSelGrp::setChosen( const DBKeySet& dbkys )
{
    if ( dbkys.isEmpty() )
	return;

    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );
    if ( isMultiChoice() )
	listfld_->chooseAll( false );

    for ( int idx=0; idx<dbkys.size(); idx++ )
    {
	const int selidx = ioobjids_.indexOf( dbkys[idx] );
	if ( selidx >= 0 )
	    listfld_->setChosen( selidx, true );
    }

    selChg( 0 );
}


bool uiIOObjSelGrp::updateCtxtIOObj()
{
    const int curitm = listfld_->currentItem();
    const int sz = listfld_->size();
    if ( ctio_.ctxt_.forread_ )
    {
	if ( isMultiChoice() )
	    return true;

	if ( curitm < 0 )
	{
	    ctio_.setObj( 0 );
	    if ( sz > 0 )
		uiMSG().error(uiStrings::phrSelect(tr("the %1 "
				 "or press Cancel").arg(mObjTypeName)));
	    return false;
	}
	PtrMan<IOObj> ioobj = getIOObj( curitm );
	if ( !ioobj )
	{
	    uiMSG().error( tr("Cannot retrieve %1 details from data store")
			    .arg(mObjTypeName) );
	    fullUpdate( -1 );
	    return false;
	}
	ctio_.setObj( ioobj->clone() );
	return true;
    }

    // from here for write only
    StringPair strpair( nmfld_->text() );
    const BufferString seltxt( strpair.first() );
    int itmidx = ioobjnms_.indexOf( seltxt.buf() );
    if ( itmidx < 0 )
	return createEntry( seltxt );

    if ( itmidx != curitm )
	setCurrent( itmidx );

    PtrMan<IOObj> ioobj = getIOObj( itmidx );
    if ( ioobj )
    {
	if ( wrtrselfld_ && !wrtrselfld_->isEmpty()
	    && !wrtrselfld_->hasSelectedTranslator(*ioobj) )
	{
	    uiMSG().error(tr("Sorry, can not change the storage type."
			     "\nIf you are sure, please remove the existing "
			     "object first"));
	    return false;
	}

	if ( ioobj->implExists(true) )
	{
	    bool allok = true;
	    if ( ioobj->implReadOnly() )
	    {
		uiMSG().error(tr("Chosen %1 is read-only").arg(mObjTypeName));
		allok = false;
	    }
	    else if ( setup_.confirmoverwrite_ && !asked2overwrite_ )
		allok = uiMSG().askOverwrite(
		tr("Overwrite existing %1?").arg(mObjTypeName));

	    if ( !allok )
		{ asked2overwrite_ = false; return false; }

	    asked2overwrite_ = true;
	}
    }

    ctio_.setObj( ioobj.release() );

    if ( ctio_.ioobj_ && wrtrselfld_ && !wrtrselfld_->isEmpty() )
    {
	wrtrselfld_->updatePars( *ctio_.ioobj_ );
	ctio_.ioobj_->commitChanges();
    }

    return true;
}


void uiIOObjSelGrp::setDefTranslator( const Translator* trl )
{
    if ( trl && wrtrselfld_ )
	wrtrselfld_->setTranslator( trl );
}


void uiIOObjSelGrp::setContext( const IOObjContext& c )
{
    ctio_.ctxt_ = c; ctio_.setObj( 0 );
    fullUpdate( -1 );
}


const IOObjContext& uiIOObjSelGrp::getContext() const
{
    return ctio_.ctxt_;
}


uiIOObjManipGroup* uiIOObjSelGrp::getManipGroup()
{
    return manipgrpsubj ? manipgrpsubj->grp_ : 0;
}


void uiIOObjSelGrp::displayManipGroup( bool yn, bool shrink)
{
    uiIOObjManipGroup* grp = getManipGroup();
    if ( grp )
	grp->display( yn, shrink );
}


void uiIOObjSelGrp::setSurveyDefaultSubsel( const char* subsel )
{
    surveydefaultsubsel_ = subsel;
}


bool uiIOObjSelGrp::fillPar( IOPar& iop ) const
{
    if ( !const_cast<uiIOObjSelGrp*>(this)->updateCtxtIOObj() || !ctio_.ioobj_ )
	return false;

    if ( !isMultiChoice() )
	iop.set( sKey::ID(), ctio_.ioobj_->key() );
    else
    {
	DBKeySet dbkys; getChosen( dbkys );
	iop.set( sKey::Size(), dbkys.size() );
	for ( int idx=0; idx<dbkys.size(); idx++ )
	    iop.set( IOPar::compKey(sKey::ID(),idx), dbkys[idx] );
    }

    return true;
}


void uiIOObjSelGrp::usePar( const IOPar& iop )
{
    if ( !isMultiChoice() )
    {
	const char* res = iop.find( sKey::ID() );
	if ( res && *res )
	    setCurrent( ioobjids_.indexOf( DBKey(res) ) );
    }
    else
    {
	int nrids;
	iop.get( sKey::Size(), nrids );
	DBKeySet dbkys;
	for ( int idx=0; idx<nrids; idx++ )
	{
	    DBKey mid;
	    if ( iop.get(IOPar::compKey(sKey::ID(),idx),mid) )
		dbkys += mid;
	}

	setChosen( dbkys );
    }
}


void uiIOObjSelGrp::fullUpdate( const DBKey& ky )
{
    int selidx = ioobjids_.indexOf( ky );
    fullUpdate( selidx );
    // Maybe a new one has been added
    if ( selidx < 0 )
	setCurrent( ioobjids_.indexOf(ky) );
}


bool uiIOObjSelGrp::doTimeSort() const
{
    return tsortbox_ ? tsortbox_->isChecked() : false;
}


void uiIOObjSelGrp::addModifTime( const IOObj& ioobj )
{
    od_int64 modiftm = File::getTimeInSeconds( ioobj.mainFileName() );
    if ( mIsUdf(modiftm) )
	modiftm = 0;
    modiftimes_ += modiftm;
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{
    const bool showicons = !Settings::common().isFalse( "Ui.Icons.ObjSel" );
    ioobjnms_.setEmpty(); dispnms_.setEmpty(); iconnms_.setEmpty();
    ioobjids_.setEmpty(); modiftimes_.setEmpty();

    const bool needtimes = doTimeSort();
    DBDirEntryList entrylist( ctio_.ctxt_, false );
    entrylist.fill( filtfld_->text() );
    DBKey selid;
    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	const IOObj& ioobj = entrylist.ioobj( idx );
	const DBKey objid( ioobj.key() );

	bool issel = idx == curidx;
	if ( curidx == -1 )
	{
	    if ( !ctio_.ioobj_ )
		issel = IOObj::isSurveyDefault( objid );
	    else
		issel = ctio_.ioobj_->key() == objid;
	}
	if ( issel )
	    selid = objid;

	ioobjids_.add( objid );
	ioobjnms_.add( ioobj.name() );
	dispnms_.add( entrylist.dispName(idx) );
	if ( needtimes )
	    addModifTime( ioobj );
	if ( showicons )
	    iconnms_.add( entrylist.iconName(idx) );
    }

    fillListBox();
    if ( selid.isValid() )
	setCurrent( selid );
    selChg( 0 );
}


void uiIOObjSelGrp::fillListBox()
{
    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );

    const BufferString prevsel( listfld_->getText() );
    listfld_->setEmpty();

    const bool dotsort = doTimeSort();
    const int sz = ioobjids_.size();
    if ( sz > 1 )
    {
	if ( dotsort )
	{
	    // make sure undefs are displayed last
	    for ( int idx=0; idx<sz; idx++ )
		if ( mIsUdf(modiftimes_[idx]) )
		    modiftimes_[idx] = 0;
	}
	BufferStringSet::idx_type* idxs = dotsort
	    ? getSortIndexes(modiftimes_,false) : dispnms_.getSortIndexes();

	ioobjids_.useIndexes( idxs );
	ioobjnms_.useIndexes( idxs );
	dispnms_.useIndexes( idxs );
	modiftimes_.useIndexes( idxs );
	iconnms_.useIndexes( idxs );
	delete [] idxs;
    }
    listfld_->addItems( dispnms_ );

    const int nricons = iconnms_.size();
    if ( nricons > 0 )
    {
	bool wantdisplay = nricons < 20;
	if ( !wantdisplay )
	{
	    BufferString previcnm( iconnms_.first() );
	    for ( int idx=1; idx<nricons; idx++ )
	    {
		const BufferString& icnm = iconnms_.get( idx );
		if ( previcnm != icnm )
		    { wantdisplay = true; break; }
	    }
	}
	if ( wantdisplay )
	{
	    for ( int idx=0; idx<iconnms_.size(); idx++ )
	    {
		const BufferString& icnm = iconnms_.get( idx );
		listfld_->setIcon( idx, icnm.isEmpty() ? "empty" : icnm.str() );
	    }
	}
    }

    if ( listfld_->isPresent(prevsel) )
	listfld_->setCurrentItem( prevsel );

    selectionChanged.trigger();
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    return ioobjids_.validIdx(idx) ? ioobjids_[idx].getIOObj() : 0;
}


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    PtrMan<IOObj> ioobj = 0;
    if ( wrtrselfld_ )
	ioobj = wrtrselfld_->mkEntry( seltxt );
    else
    {
	CtxtIOObj ctio( ctio_.ctxt_ ); ctio.setName( seltxt );
	ctio.fillObj( false ); ioobj = ctio.ioobj_;
    }
    if ( !ioobj )
    {
	uiMSG().error( uiStrings::phrCannotCreate(
				tr("%1 with this name").arg(mObjTypeName) ));
	return false;
    }

    const bool showicons = !Settings::common().isFalse( "Ui.Icons.ObjSel" );
    ioobjnms_.add( ioobj->name() );
    dispnms_.add( ioobj->name() );
    ioobjids_.add( ioobj->key() );
    if ( doTimeSort() )
	addModifTime( *ioobj );
    if ( showicons )
    {
	PtrMan<Translator> transl = ioobj->createTranslator();
	BufferString iconnm;
	if ( transl )
	    iconnm.set( transl->iconName() );
	iconnms_.add( iconnm.buf() );
    }

    fillListBox();

    listfld_->setCurrentItem( ioobj->name() );
    if ( nmfld_ && ioobj->name() != seltxt )
	nmfld_->setText( ioobj->name() );

    ctio_.setObj( ioobj->clone() );
    selectionChanged.trigger();
    return true;
}


IOObj* uiIOObjSelGrp::updStatusBarInfo( bool setnmfld )
{
    BufferString info;
    const int nrchosen = nrChosen();
    const int idx = listfld_->currentItem();
    IOObj* ret = getIOObj( idx );
    ctio_.setObj( ret ? ret->clone() : 0 );
    if ( isMultiChoice() && nrchosen>1 )
    {
	info.set( nrchosen ).add( "/" ).add( listfld_->size() )
			    .add( " chosen" );
    }
    else
    {
	if ( setnmfld && nmfld_ && ret )
	    nmfld_->setText( ret->name().buf() );
	info = getLimitedDisplayString( !ret ? "" :
			 ret->fullUserExpr(ctio_.ctxt_.forread_), 40, false );
    }

    triggerStatusMsg( info );
    return ret;
}


void uiIOObjSelGrp::triggerStatusMsg( const char* txt )
{
    CBCapsule<const char*> caps( txt, this );
    newStatusMsg.trigger( &caps );
}


void uiIOObjSelGrp::setInitial( CallBacker* )
{
    if ( !ctio_.ctxt_.forread_ )
    {
	PtrMan<IOObj> ioobj = 0;
	if ( ctio_.ioobj_ )
	    nmfld_->setText( ctio_.ioobj_->name() );

	FixedString presetnm = nmfld_->text();
	if ( !presetnm.isEmpty() && listfld_->isPresent(presetnm) )
	{
	    listfld_->setCurrentItem( presetnm );
	    if ( wrtrselfld_ )
	    {
		if ( !ioobj )
		    ioobj = currentID().getIOObj();
		if ( ioobj )
		    wrtrselfld_->use( *ioobj );
	    }
	}
    }

    mAttachCB( listfld_->selectionChanged, uiIOObjSelGrp::selChg );
    mAttachCB( listfld_->itemChosen, uiIOObjSelGrp::choiceChg );
    mAttachCB( listfld_->deleteButtonPressed, uiIOObjSelGrp::delPress );
    mAttachCB( manipgrpsubj->itemChanged, uiIOObjSelGrp::itemChg );

    if ( ctio_.ctxt_.forread_ )
	selChg( 0 );

    sortChgCB( 0 );
}


void uiIOObjSelGrp::selChg( CallBacker* )
{
    PtrMan<IOObj> ioobj = updStatusBarInfo( true );
    if ( mkdefbut_ )
    {
	mkdefbut_->setSensitive( listfld_->size() );
	const bool isdef = ioobj && IOObj::isSurveyDefault( ioobj->key() );
	if ( ioobj && !isdef )
	{
	    uiString tt = tr("Set '%1' as default").arg(ioobj->name());
	    mkdefbut_->setToolTip( tt );
	}
	else if ( isdef )
	{
	    uiString deftt = tr("'%1' is default object").arg(ioobj->name());
	    mkdefbut_->setToolTip( deftt );
	}
	else
	    mkdefbut_->setToolTip( uiStrings::phrSetAs(uiStrings::sDefault()) );
    }

    if ( wrtrselfld_ && ioobj )
	wrtrselfld_->use( *ioobj );

    selectionChanged.trigger();
}


void uiIOObjSelGrp::itemChg( CallBacker* )
{
    itemChanged.trigger();
}


void uiIOObjSelGrp::choiceChg( CallBacker* )
{
    delete updStatusBarInfo( false );
    itemChosen.trigger();
}


void uiIOObjSelGrp::refreshCB( CallBacker* )
{
    fullUpdate( currentID() );
}


void uiIOObjSelGrp::orderChgCB( CallBacker* )
{
    fullUpdate( -2 );
}


void uiIOObjSelGrp::sortChgCB( CallBacker* cb )
{
    const bool dotsort = doTimeSort();
    if ( dotsort != didtsort_ )
    {
	Settings::common().setYN( sKeyTimeSort, dotsort );
	Settings::common().write();
	didtsort_ = dotsort;
    }

    orderChgCB( cb );
}


void uiIOObjSelGrp::objInserted( CallBacker* cb )
{
    mCBCapsuleUnpack( DBKey, ky, cb );
    if ( ky.isValid() )
	fullUpdate( ky );
}


void uiIOObjSelGrp::nameAvCB( CallBacker* )
{
    if ( !nmfld_ ) return;
    const char* newnm = wrtrselfld_->suggestedName();
    if ( newnm && *newnm )
	nmfld_->setText( newnm );
}


void uiIOObjSelGrp::delPress( CallBacker* )
{
    if ( manipgrpsubj && manipgrpsubj->manipgrp_ )
	manipgrpsubj->manipgrp_->triggerButton( uiManipButGrp::Remove );
}


void uiIOObjSelGrp::makeDefaultCB(CallBacker*)
{
    PtrMan<IOObj> ioobj = currentID().getIOObj();
    if ( !ioobj )
	return;

    ioobj->setSurveyDefault( surveydefaultsubsel_.str() );

    const int cursel = currentItem();
    DBKeySet chosendbkys;
    getChosen( chosendbkys );
    fullUpdate( -2 );
    setChosen( chosendbkys );
    setCurrent( cursel );
}


void uiIOObjSelGrp::newOutputNameCB( CallBacker* )
{
    const int deftransidx = ctio_.ctxt_.trgroup_->defTranslIdx();
    const Translator* deftrans = ctio_.ctxt_.trgroup_->templates()[deftransidx];
    PtrMan<IOObj> curioobj = currentID().getIOObj();
    const Translator* selectedtrans =
		wrtrselfld_ ? wrtrselfld_->selectedTranslator() : 0;
    const bool translatorchanged = curioobj && selectedtrans
			 ? curioobj->translator() != selectedtrans->userName()
			 : false;

    if ( !translatorchanged && wrtrselfld_ )
    {
	if ( curioobj && wrtrselfld_->hasWriteOpts() )
	{
	    uiIOObjSelWriteTranslator currentwrtselfld(this,ctio_.ctxt_,true);
	    currentwrtselfld.use( *curioobj );
	    if ( wrtrselfld_->hasSameWriteOpts(currentwrtselfld) )
		wrtrselfld_->resetPars();
	}

	wrtrselfld_->setTranslator( deftrans );
	updateCtxtIOObj();
    }
}


void uiIOObjSelGrp::readChoiceDone( CallBacker* )
{
    if ( !lbchoiceio_ ) return;

    DBKeySet dbkys;
    for ( int idx=0; idx<lbchoiceio_->chosenKeys().size(); idx++ )
    {
	const BufferString kystr( lbchoiceio_->chosenKeys().get(idx) );
	if ( DBKey::isValidString(kystr) )
	    dbkys += DBKey( kystr );
    }
    setChosen( dbkys );
}


void uiIOObjSelGrp::writeChoiceReq( CallBacker* )
{
    if ( !lbchoiceio_ ) return;

    lbchoiceio_->keys().setEmpty();
    for ( int idx=0; idx<ioobjids_.size(); idx++ )
	lbchoiceio_->keys().add( ioobjids_[idx].toString() );
}


void uiIOObjSelGrp::ctxtChgCB( CallBacker* )
{
    const BufferString withctxtfilter( setup_.withctxtfilter_ );
    if ( ctxtfiltfld_ )
    {
	const int curitm = ctxtfiltfld_->currentItem();
	if ( withctxtfilter == ctio_.ctxt_.trgroup_->groupName() )
	{
	    if ( curitm <= 0 )
		ctio_.ctxt_.toselect_.allowtransls_ = BufferString::empty();
	    else
	    {
		BufferString currnm = ctxtfiltfld_->itemText( curitm );
		if ( currnm.isEqual(dGBToDispStorageStr()) )
		    currnm = mDGBKey;
		ctio_.ctxt_.toselect_.allowtransls_ = currnm;
	    }
	}
	else
	{
	    if ( curitm <= 0 )
		ctio_.ctxt_.toselect_.require_.removeWithKey( withctxtfilter );
	    else
		ctio_.ctxt_.toselect_.require_.set( withctxtfilter,
					    ctxtfiltfld_->itemText( curitm ) );
	}
	fullUpdate( -2 );
    }
}
