/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobjselgrp.h"
#include "uiioobjinserter.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "filepath.h"
#include "filesystemwatcher.h"
#include "globexpr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "settings.h"
#include "separstr.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitoolbutton.h"


static const char* NoIconNm = "empty";
static OD::Color sNotOKColor( OD::Color::Red().lighter(0.9f) );

static bool requireIcon()
{
    mDefineStaticLocalObject(const bool, icsel,
	= Settings::common().isFalse("Ui.Icons.ObjSel"));
    return !icsel;
}


// EntryData
EntryData::EntryData( const MultiID& mid )
{
    mid_ = mid;
}


EntryData::EntryData( const MultiID& mid, const char* objnm,
	    const char* dispnm, const char* icnnm, bool isdef )
{
    mid_ = mid;
    objnm_ = objnm;
    dispnm_ = dispnm;
    icnnm_ = icnnm;
    isdef_ = isdef;
}


EntryData::~EntryData()
{}


void EntryData::setIconName( const char* iconnm )
{
    icnnm_ = iconnm;
}


void EntryData::setDisplayName( const char* dispnm )
{
    dispnm_ = dispnm;
}


void EntryData::setObjName( const char* objnm )
{
    objnm_ = objnm;
}


// EntryDataSet
EntryDataSet::EntryDataSet()
{}


EntryDataSet::~EntryDataSet()
{}


EntryDataSet& EntryDataSet::add( const MultiID& mid, bool isdef )
{
    if ( mid.isUdf() || livemids_.isPresent(mid) )
	return *this;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return *this;

    const BufferString& nm = ioobj->name();
    BufferString icnnm = NoIconNm;
    if ( requireIcon() )
    {
	PtrMan<Translator> transl = ioobj->createTranslator();
	if ( transl )
	    icnnm = transl->iconName();
    }

    auto* data = new EntryData( mid, nm, nm, icnnm, isdef );
    *this += data;
    livemids_.add( mid );
    if ( isdef )
	defaultidxs_.add( this->size()-1 );

    return *this;
}


EntryDataSet& EntryDataSet::add( const MultiID& mid, const BufferString& objnm,
			const BufferString& dispnm, bool isdef )
{
    if ( mid.isUdf() || livemids_.isPresent(mid) )
	return *this;

    BufferString icnnm = NoIconNm;
    if ( requireIcon() )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	PtrMan<Translator> transl = ioobj ? ioobj->createTranslator() : nullptr;
	if ( transl )
	    icnnm = transl->iconName();
    }

    EntryData* data = new EntryData( mid, objnm, dispnm, icnnm, isdef );
    *this += data;
    livemids_.add( mid );
    if ( isdef )
	defaultidxs_.add( this->size() - 1 );

    return *this;
}


void EntryDataSet::erase()
{
    ManagedObjectSet::erase();
    livemids_.setEmpty();
    defaultidxs_.setEmpty();
}


EntryDataSet& EntryDataSet::removeMID( const MultiID & mid )
{
    if ( mid.isUdf() )
	return *this;

    const int idx = livemids_.indexOf( mid );
    if ( !livemids_.validIdx(idx) )
	return *this;

    this->removeSingle( idx );
    livemids_.removeSingle( idx );
    return *this;
}


EntryDataSet& EntryDataSet::updateMID( const MultiID& mid, EntryData* ed )
{
    if ( mid.isUdf() )
	return *this;

    const int idx = livemids_.indexOf( mid );
    if ( !livemids_.validIdx(idx) )
	return *this;

    if ( ed != getDataFor(mid) )
	this->replace( idx, ed );

    return *this;
}


const TypeSet<MultiID>& EntryDataSet::getIOObjIds() const
{
    return livemids_;
}


const TypeSet<int>& EntryDataSet::getDefaultIdxs( bool reread ) const
{
    if ( reread )
    {
	defaultidxs_.setEmpty();
	for ( int idx=0; idx<size(); idx++ )
	{
	    const auto* ed = get( idx );
	    if ( ed->isDef() )
		defaultidxs_.add( idx );
	}
    }

    return defaultidxs_;
}


BufferStringSet EntryDataSet::getIOObjNms() const
{
    BufferStringSet objnms;
    for ( const EntryData* ed : *this )
	objnms.add( ed->getObjNm() );

    return objnms;
}


BufferStringSet EntryDataSet::getDispNms() const
{
    BufferStringSet dispnms;
    for ( const EntryData* ed : *this )
	dispnms.add( ed->getDispNm( ));

    return dispnms;
}



BufferStringSet EntryDataSet::getIconNms() const
{
    BufferStringSet iconnms;
    for (const EntryData* ed : *this)
	iconnms.add( ed->getIcnNm() );

    return iconnms;
}

int EntryDataSet::indexOfMID( const MultiID& mid ) const
{
    return livemids_.indexOf( mid );
}


int EntryDataSet::indexOfNm( const BufferString& nm, bool isdispnm ) const
{
    const BufferStringSet& nms = isdispnm ? getDispNms() : getIOObjNms();
    return nms.indexOf( nm );
}


const EntryData* EntryDataSet::getDataFor( const MultiID& mid ) const
{
    const int idx = livemids_.indexOf( mid );
    if ( mid.isUdf() || idx < 0 )
	return nullptr;

    return this->get( idx );
}


EntryData* EntryDataSet::getDataFor(const MultiID& mid)
{
    int idx = livemids_.indexOf( mid );
    if ( mid.isUdf() || idx < 0 )
    {
	return nullptr;
    }

    return this->get( idx );
}


// uiIOObjSelGrpManipSubj
static const char* dGBToDispStorageStr()    { return "OpendTect";  }

class uiIOObjSelGrpManipSubj : public uiIOObjManipGroupSubj
{ mODTextTranslationClass(uiIOObjSelGrpManipSubj);
public:

uiIOObjSelGrpManipSubj( uiIOObjSelGrp* sg )
    : uiIOObjManipGroupSubj(sg->listfld_->box())
    , selgrp_(sg)
{
    mAttachCB( selgrp_->selectionChanged, uiIOObjSelGrpManipSubj::selChg );
    mAttachCB( selgrp_->itemChosen, uiIOObjSelGrpManipSubj::selChg );
}


~uiIOObjSelGrpManipSubj()
{
    detachAllNotifiers();
}


MultiID currentID() const override
{
    return selgrp_->currentID();
}


void getChosenIDs( TypeSet<MultiID>& mids ) const override
{
    selgrp_->getChosen( mids );
}


void getChosenNames( BufferStringSet& nms ) const override
{
    selgrp_->getChosen( nms );
}


bool isEntryOK( const MultiID& id ) const override
{
    return selgrp_->isEntryOK( id );
}


const char* defExt() const override
{
    return selgrp_->ctio_.ctxt_.trgroup_->defExtension();
}

BufferStringSet names() const override
{
    return selgrp_->dataset_.getIOObjNms();
}

void chgsOccurred() override
{
    selgrp_->fullUpdate( selgrp_->listfld_->currentItem() );
}

void itemInitRead( const IOObj* obj ) override
{
    selgrp_->itemInitRead.trigger( obj );
}

void selChg( CallBacker* )
{
    if ( manipgrp_ )
	manipgrp_->selChg();
}

void launchLocate( const MultiID& key ) override
{
    selgrp_->launchLocate.trigger( key );
}

    uiIOObjSelGrp*	selgrp_;
    uiIOObjManipGroup*	manipgrp_ = nullptr;

}; // class uiIOObjSelGrpManipSubj



// uiIOObjSelGrp
#define muiIOObjSelGrpConstructorCommons \
      uiGroup(p) \
    , selectionChanged(this) \
    , itemChosen(this) \
    , newStatusMsg(this) \
    , listUpdated(this) \
    , zDomainChanged(this) \
    , itemAdded(this) \
    , itemRemoved(this) \
    , itemInitRead(this) \
    , itemChanged(this) \
    , launchLocate(this) \
    , ctio_(*new CtxtIOObj(c))


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
    if ( !ctio_.ctxt_.forread_ )
	setup_.choicemode( OD::ChooseOnlyOne );

    IOM().to( ctio_.ctxt_.getSelKey() );
    mkTopFlds( seltxt );
    if ( !ctio_.ctxt_.forread_ )
	mkWriteFlds();
    if ( ctio_.ctxt_.maydooper_ )
	mkManipulators();

    fswatcher_ = new FileSystemWatcher;
    const FilePath fp( IOM().curDirName(), ".omf" );
    fswatcher_->addFile( fp.fullPath() );
    mAttachCB( fswatcher_->fileChanged, uiIOObjSelGrp::omfChgCB );

    setHAlignObj( topgrp_ );
    mAttachCB( postFinalize(), uiIOObjSelGrp::initGrpCB );
}


void uiIOObjSelGrp::omfChgCB( CallBacker* )
{
    if ( grpobj_->visible() )
	fullUpdate( -1 );
}


uiObject* uiIOObjSelGrp::getFilterFieldAttachObj()
{
    return filtfld_->attachObj();
}


void uiIOObjSelGrp::mkTopFlds( const uiString& seltxt )
{
    topgrp_ = new uiGroup( this, "Top group" );

    uiListBox::Setup su( setup_.choicemode_, seltxt );
    listfld_ = new uiListBox( topgrp_, su, "Objects" );

    filtfld_ = new uiGenInput( listfld_, uiStrings::sFilter() );
    filtfld_->setElemSzPol( uiObject::SmallVar );
    mAttachCB( filtfld_->updateRequested, uiIOObjSelGrp::filtChg );

    uiLabeledComboBox* lastuilcb = nullptr;
    const FileMultiString withctxtfilters( setup_.withctxtfilter_ );
    for ( int ifilt=0; ifilt<withctxtfilters.size(); ifilt++ )
    {
	const bool noall = withctxtfilters[ifilt] == ZDomain::sKeyNoAll();
	const BufferString withctxtfilter = noall ? ZDomain::sKey() :
					(const char*) withctxtfilters[ifilt];
	if ( withctxtfilter.isEmpty() )
	    continue;

	const bool iszdomain = withctxtfilter == ZDomain::sKey();
	const bool istype = withctxtfilter == sKey::Type();
	const bool istransl = !iszdomain && !istype;
	const uiString lblstr( iszdomain
			? tr("Z Domain")
			: (istype ? uiStrings::sType()
			      : uiStrings::phrJoinStrings(uiStrings::sFile(),
							  uiStrings::sType())));
	const IODir iodir( ctio_.ctxt_.getSelKey() );
	const IODirEntryList entrylist( iodir, ctio_.ctxt_ );
	BufferStringSet valstrs = entrylist.getValuesFor( withctxtfilter );
	const int idx = valstrs.indexOf( mDGBKey );
	if ( idx > -1 )
	    valstrs.get(idx).set( dGBToDispStorageStr() );
	//This is strictly for display purpose without changing the key
	if ( valstrs.size()>1 )
	{
	    FileMultiString fms( withctxtfilter.str() );
	    BufferString filterval;
	    if ( istransl )
		filterval = ctio_.ctxt_.toselect_.allowtransls_;
	    else
		filterval = ctio_.ctxt_.toselect_.require_.find(withctxtfilter);

	    if ( !filterval.isEmpty() )
		fms.add( filterval.str() );

	    valstrs.sort();
	    auto* firstline = new BufferString( sKey::All() );
	    if ( !noall )
		valstrs.insertAt( firstline, 0 );

	    auto* ctxtfiltfld = new uiLabeledComboBox( topgrp_, valstrs,
						       lblstr, fms.str() );
	    uiComboBox* box = ctxtfiltfld->box();
	    box->setHSzPol( uiObject::SmallVar );
	    if ( noall )
		box->setCurrentItem( SI().zDomain().key() );

	    if ( lastuilcb )
		ctxtfiltfld->attach( rightOf, lastuilcb );
	    else
		listfld_->attach( alignedBelow, ctxtfiltfld );

	    lastuilcb = ctxtfiltfld;
	    ctxtfiltfld_ = box;

	    if ( istransl )
		mAttachCB( box->selectionChanged,
			   uiIOObjSelGrp::ctxtFileTypeChgCB );
	    else if ( iszdomain )
	    {
		mAttachCB( box->selectionChanged,
			   uiIOObjSelGrp::ctxtZDomainChgCB );
		ctxtZDomainChgCB( box );
	    }
	    else
		mAttachCB( box->selectionChanged,
		       uiIOObjSelGrp::ctxtTypeChgCB );
	}
    }

    listfld_->box()->attach( rightAlignedBelow, getFilterFieldAttachObj() );

    topgrp_->setHAlignObj( listfld_ );

    listfld_->setName( "Objects list" );
    listfld_->box()->setPrefHeightInChar( 8 );
    listfld_->setHSzPol( uiObject::Wide );
    if ( isMultiChoice() )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_,
					     ctio_.ctxt_.objectTypeName() );
	mAttachCB( lbchoiceio_->readDone, uiIOObjSelGrp::readChoiceDone );
	mAttachCB( lbchoiceio_->storeRequested, uiIOObjSelGrp::writeChoiceReq );
    }
}


void uiIOObjSelGrp::mkWriteFlds()
{
    auto* wrgrp = new uiGroup( this, "Write group" );
    wrtrselfld_ = nullptr;
    if ( setup_.withwriteopts_ )
    {
	wrtrselfld_ = new uiIOObjSelWriteTranslator( wrgrp, ctio_,
						setup_.trsnotallwed_, true );
	mAttachCB( wrtrselfld_->suggestedNameAvailble,
						    uiIOObjSelGrp::nameAvCB );
    }

    nmfld_ = new uiGenInput( wrgrp, uiStrings::sName() );
    nmfld_->setDefaultTextValidator();
    nmfld_->setElemSzPol( uiObject::SmallMax );
    nmfld_->setStretch( 2, 0 );
    mAttachCB( nmfld_->valueChanged, uiIOObjSelGrp::newOutputNameCB );
    if ( wrtrselfld_ && !wrtrselfld_->isEmpty() )
	nmfld_->attach( alignedBelow, wrtrselfld_ );

    wrgrp->setHAlignObj( nmfld_ );
    wrgrp->attach( alignedBelow, topgrp_ );

    const StringPair ctionm( ctio_.name() );
    const BufferString& nm = ctionm.first();
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
    manipgrpsubj_ = new uiIOObjSelGrpManipSubj( this );
    manipgrpsubj_->manipgrp_ = new uiIOObjManipGroup( *manipgrpsubj_,
						     setup_.allowreloc_,
						     setup_.allowremove_ );
    if ( setup_.allowsetdefault_ )
    {
	mkdefbut_ = manipgrpsubj_->manipgrp_->addButton(
	    "makedefault", uiStrings::phrSetAs(uiStrings::sDefault()),
	    mCB(this,uiIOObjSelGrp,makeDefaultCB) );
    }

    if ( !setup_.withinserters_ || uiIOObjInserter::allDisabled() )
	return;

    if ( !ctio_.ctxt_.forread_ ||
	 !uiIOObjInserter::isPresent(*ctio_.ctxt_.trgroup_) )
	return;

    const BufferStringSet nms;
    uiButton* but = uiIOObjInserter::createInsertButton( listfld_, ctio_,
							inserters_, nms );
    for ( auto* inserter : inserters_ )
	mAttachCB( inserter->objInserterd, uiIOObjSelGrp::objInserted );

    if ( but )
	but->attach( centeredLeftOf, listfld_->box() );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    detachAllNotifiers();
    deepErase( inserters_ );
    if ( manipgrpsubj_ )
	delete manipgrpsubj_->manipgrp_;

    delete manipgrpsubj_;
    delete ctio_.ioobj_;
    delete &ctio_;
    delete lbchoiceio_;
    delete fswatcher_;
}


bool uiIOObjSelGrp::isEmpty() const { return listfld_->isEmpty(); }
int uiIOObjSelGrp::size() const { return listfld_->size(); }
int uiIOObjSelGrp::currentItem() const { return listfld_->currentItem(); }
int uiIOObjSelGrp::nrChosen() const { return listfld_->nrChosen(); }
bool uiIOObjSelGrp::isChosen( int idx ) const { return listfld_->isChosen(idx);}
void uiIOObjSelGrp::setChosen( int idx, bool yn )
{ listfld_->setChosen(idx,yn);}
void uiIOObjSelGrp::chooseAll( bool yn ) { listfld_->chooseAll(yn);}


MultiID uiIOObjSelGrp::currentID() const
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 || !dataset_.validIdx(selidx) )
	return MultiID::udf();

    const EntryData* ed = dataset_.get( selidx );
    return ed ? ed->getMID() : MultiID::udf();
}


MultiID uiIOObjSelGrp::chosenID( int objnr ) const
{
    const TypeSet<MultiID>& mids = getIOObjIds();
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( isChosen(idx) )
	    objnr--;

	if ( objnr < 0 )
	    return mids.validIdx(idx) ? mids[idx] : MultiID::udf();
    }

    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrChosen()="; msg += nrChosen();
    pErrMsg( msg );
    return MultiID::udf();
}


void uiIOObjSelGrp::getChosen( TypeSet<MultiID>& mids ) const
{
    mids.setEmpty();
    const int nrchosen = nrChosen();
    for ( int idx=0; idx<nrchosen; idx++ )
	mids += chosenID( idx );
}


void uiIOObjSelGrp::getChosen( BufferStringSet& nms ) const
{
    nms.setEmpty();
    const BufferStringSet& objnms = dataset_.getIOObjNms();
    const int nrobjs = objnms.size();
    if ( nrobjs != listfld_->size() )
    {
	pErrMsg("Should not happen");
	return;
    }

    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    nms.add( objnms.get( idx ) );
    }
}


void uiIOObjSelGrp::setCurrent( int curidx )
{
    const int sz = dataset_.size();
    if ( curidx >= sz )
	curidx = sz - 1;

    listfld_->setCurrentItem( curidx );
    selectionChanged.trigger();
}


void uiIOObjSelGrp::setIsBad( int idx )
{
    if ( !listfld_->validIdx(idx) )
	return;

    listfld_->setColor( idx, sNotOKColor );
}


bool uiIOObjSelGrp::isEntryOK( const MultiID& mid ) const
{
    const int idx = dataset_.indexOfMID( mid );
    if ( !listfld_->validIdx(idx) )
	return false;

    const bool notok = listfld_->getColor( idx ) == sNotOKColor
		     || listfld_->isMarked( idx );
    return !notok;
}


void uiIOObjSelGrp::setCurrent( const MultiID& mid )
{
    const int idx = dataset_.indexOfMID( mid );
    if ( idx >= 0 )
	setCurrent( idx );
}


void uiIOObjSelGrp::setChosen( const TypeSet<MultiID>& mids )
{
    if ( mids.isEmpty() )
	return;

    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );
    if ( isMultiChoice() )
	listfld_->chooseAll( false );

    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const int selidx = dataset_.indexOfMID( mids[idx] );
	if ( selidx >= 0 )
	    listfld_->setChosen( selidx, true );
    }

    selChg( nullptr );
}


bool uiIOObjSelGrp::updateCtxtIOObj()
{
    const int curitm = listfld_->currentItem();
    const int sz = listfld_->size();
    const char* objtypenm = ctio_.ctxt_.objectTypeName();
    if ( ctio_.ctxt_.forread_ )
    {
	if ( isMultiChoice() )
	    return true;

	if ( curitm < 0 )
	{
	    ctio_.setObj( nullptr );
	    if ( sz > 0 )
		uiMSG().error(tr("Please select the %1 "
				 "or press Cancel").arg(objtypenm));
	    return false;
	}

	PtrMan<IOObj> ioobj = getIOObj( curitm );
	if ( !ioobj )
	{
	    uiMSG().error(tr("Internal error: "
			     "Cannot retrieve %1 details from data store")
			.arg(objtypenm));
	    // TODO: Check if setting to root is OK
	    IOM().toRoot();
	    fullUpdate( -1 );
	    return false;
	}
	ctio_.setObj( ioobj->clone() );
	return true;
    }

    // from here for write only
    StringPair nm( nmfld_->text() );
    const BufferString& seltxt = nm.first();
    const int itmidx = dataset_.indexOfNm( seltxt.buf(), false );
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
	    uiMSG().error( tr("Sorry, can not change the storage type."
			    "\nIf you are sure, please remove the existing "
			    "object first") );
	    return false;
	}

	if ( ioobj->implExists(true) )
	{
	    bool allok = true;
	    if ( ioobj->implReadOnly() )
	    {
		uiMSG().error(tr("Chosen %1 is read-only").arg(objtypenm));
		allok = false;
	    }
	    else if ( setup_.confirmoverwrite_ && !asked2overwrite_ )
		allok = uiMSG().askOverwrite(
		tr("Overwrite existing %1?").arg(objtypenm));

	    if ( !allok )
		{ asked2overwrite_ = false; return false; }

	    asked2overwrite_ = true;
	}
    }

    ctio_.setObj( ioobj.release() );

    if ( ctio_.ioobj_ && wrtrselfld_ && !wrtrselfld_->isEmpty() )
    {
	wrtrselfld_->updatePars( *ctio_.ioobj_ );
	IOM().commitChanges( *ctio_.ioobj_ );
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
    ctio_.ctxt_ = c;
    ctio_.setObj( nullptr );
    fullUpdate( -1 );
}


const IOObjContext& uiIOObjSelGrp::getContext() const
{
    return ctio_.ctxt_;
}


uiIOObjManipGroup* uiIOObjSelGrp::getManipGroup()
{
    return manipgrpsubj_ ? manipgrpsubj_->grp_ : nullptr;
}


void uiIOObjSelGrp::displayManipGroup( bool yn, bool shrink )
{
    if ( manipgrpsubj_ && manipgrpsubj_->grp_ )
	manipgrpsubj_->grp_->display( yn, shrink );
}


bool uiIOObjSelGrp::fillPar( IOPar& iop ) const
{
    if ( !const_cast<uiIOObjSelGrp*>(this)->updateCtxtIOObj() || !ctio_.ioobj_ )
	return false;

    if ( !isMultiChoice() )
	iop.set( sKey::ID(), ctio_.ioobj_->key() );
    else
    {
	TypeSet<MultiID> mids; getChosen( mids );
	iop.set( sKey::Size(), mids.size() );
	for ( int idx=0; idx<mids.size(); idx++ )
	    iop.set( IOPar::compKey(sKey::ID(),idx), mids[idx] );
    }

    return true;
}


void uiIOObjSelGrp::usePar( const IOPar& iop )
{
    if ( !isMultiChoice() )
    {
	MultiID mid;
	iop.get( sKey::ID(), mid );
	if ( mid.isUdf() )
	    return;

	const int selidx = dataset_.indexOfMID( mid );
	if ( selidx >= 0 )
	    setCurrent( selidx );
    }
    else
    {
	int nrids;
	iop.get( sKey::Size(), nrids );
	TypeSet<MultiID> mids;
	for ( int idx=0; idx<nrids; idx++ )
	{
	    MultiID mid;
	    if ( iop.get(IOPar::compKey(sKey::ID(),idx),mid) )
		mids += mid;
	}

	setChosen( mids );
    }
}


void uiIOObjSelGrp::fullUpdate( const MultiID& ky )
{
    int selidx = dataset_.indexOfMID( ky );
    fullUpdate( selidx );
    // Maybe a new one has been added
    if ( selidx < 0 )
    {
	selidx = dataset_.indexOfMID( ky );
	if ( selidx >= 0 )
	    setCurrent( selidx );
    }
}


void uiIOObjSelGrp::addEntry( const MultiID& mid )
{
    const int selidx = dataset_.indexOfMID( mid );
    if ( selidx < 0 )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    return;

	const bool isdef = setup_.allowsetdefault_
				    ? IOObj::isSurveyDefault(mid) : false;
	dataset_.add( mid, isdef );
    }

    addEntryToListBox( mid );
    itemAdded.trigger( mid );
}


void uiIOObjSelGrp::removeEntry( const MultiID& mid )
{
    dataset_.removeMID( mid );
    fillListBox();
    setCurrent( 0 );
    itemRemoved.trigger( mid );
    selChg( nullptr );
}


void uiIOObjSelGrp::updateEntry( const MultiID& mid, const BufferString& objnm,
			const BufferString& dispnm, const BufferString& icnnm )
{
    EntryData* ed = dataset_.getDataFor( mid );
    if ( !ed )
	return;

    BufferStringSet chngs;
    chngs.add( ed->getObjNm() );
    ed->setDisplayName( dispnm.buf() );
    ed->setIconName( icnnm.buf() );
    ed->setObjName( objnm.buf() );
    setCurrent( 0 );
    chngs.add( objnm );
    itemChanged.trigger( chngs );
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{
    const IODir iodir( ctio_.ctxt_.getSelKey() );
    IODirEntryList del( iodir, ctio_.ctxt_ );
    BufferString nmflt = filtfld_->text();
    GlobExpr::validateFilterString( nmflt );
    del.fill( iodir, nmflt );

    dataset_.setEmpty();
    BufferStringSet defkeys;
    TypeSet<int> curidxs;
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	// 'uiIOObjEntryInfo'
	BufferString dispnm( del[idx]->name() );
	BufferString ioobjnm;
	MultiID objid;
	bool isdef = false;
	if ( ioobj )
	{
	    objid = ioobj->key();
	    const bool issel = ctio_.ioobj_ && ctio_.ioobj_->key() == objid;
	    if ( setup_.allowsetdefault_ )
	    {
		const OD::String& grpnm = ioobj->group();
		const TranslatorGroup* trl = !grpnm.isEmpty() &&
			TranslatorGroup::hasGroup( grpnm.buf() )
		    ? &TranslatorGroup::getGroup( ioobj->group())
		    : nullptr;
		if ( trl )
		{
		    const BufferString keynm =
				trl->getSurveyDefaultKey( ioobj );
		    isdef = SI().pars().isPresent(keynm) &&
			    SI().pars().find(keynm) == objid.toString();
		    if ( isdef )
			defkeys.add( keynm.buf() );
		}
		else
		{
		    isdef = IOObj::isSurveyDefault( objid );
		    if ( isdef )
			defkeys.add( BufferString::empty() );
		}
	    }

	    ioobjnm = ioobj->name();
	    dispnm = ioobj->name();

	    if ( curidx < 0 &&
		 (issel || (isdef && !ctio_.ioobj_ && ctio_.ctxt_.forread_)) )
	    {
		curidxs += idx;
		if ( defkeys.size() < curidxs.size() )
		    defkeys.add( BufferString::empty() );
	    }
	}
	else
	    ioobjnm = dispnm;

	//TODO cleaner is to put this into one object: uiIOObjEntryInfo
	dataset_.add( objid, ioobjnm, dispnm, isdef );
    }

    if ( defkeys.isEmpty() )
    {
	if ( !curidxs.isEmpty() )
	    curidx = curidxs.first();
    }
    else
    {
	//Find the shortest non-empty key
	int keyidx = -1; int keysz = mUdf(int);
	for ( int idx=0; idx<defkeys.size(); idx++ )
	{
	    const BufferString& defkey = defkeys.get( idx );
	    if ( !defkey.isEmpty() && defkey.size() < keysz )
	    {
		keyidx = idx;
		keysz = defkey.size();
	    }
	}

	if ( curidxs.validIdx(keyidx) )
	    curidx = curidxs[keyidx];
	else if ( !curidxs.isEmpty() )
	    curidx = curidxs.first();
    }

    fillListBox();
    setCurrent( curidx );
    listUpdated.trigger();
    selChg( nullptr );
}


void uiIOObjSelGrp::addEntryToListBox( const MultiID& mid )
{
    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );
    const EntryData* ed = dataset_.getDataFor( mid );
    if ( !ed )
	return;

    const BufferString& dispnm = ed->getDispNm();
    listfld_->addItem( dispnm );
    if ( requireIcon() )
    {
	BufferString icnm( ed->getIcnNm() );
	if ( icnm.isEmpty() )
	    icnm = NoIconNm;

	const int idx = listfld_->indexOf( dispnm );
	listfld_->setIcon( idx, icnm );
	listfld_->setDefaultColor( idx );
    }
}


void uiIOObjSelGrp::fillListBox()
{
    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );

    listfld_->setEmpty();
    listfld_->addItems( dataset_.getDispNms() );
    listfld_->resizeHeightToContents();
    if ( requireIcon() )
    {
	const BufferStringSet& iconnm = dataset_.getIconNms();
	for ( int idx=0; idx< iconnm.size(); idx++ )
	{
	    BufferString icnm( iconnm.get(idx) );
	    if ( icnm.isEmpty() )
		icnm = NoIconNm;

	    listfld_->setIcon( idx, icnm );
	    listfld_->setDefaultColor( idx );
	}
    }

    const TypeSet<int>& defaultidxs = dataset_.getDefaultIdxs();
    for ( const auto& idx : defaultidxs )
	listfld_->setBold( idx, true );

    selectionChanged.trigger();
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    if ( idx < 0 || idx >= dataset_.size() )
	return nullptr;

    const MultiID& mid = dataset_.get( idx )->getMID();
    return mid.isUdf() ? nullptr : IOM().get( mid );
}


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    PtrMan<IOObj> ioobj = nullptr;
    if ( wrtrselfld_ )
	ioobj = wrtrselfld_->mkEntry( seltxt );
    else
    {
	CtxtIOObj ctio( ctio_.ctxt_ );
	ctio.setName( seltxt );
	ctio.fillObj( false );
	ioobj = ctio.ioobj_;
    }

    if ( !ioobj )
    {
	uiMSG().error( uiStrings::phrCannotCreate(
		tr("%1 with this name").arg(ctio_.ctxt_.objectTypeName())) );
	return false;
    }

    dataset_.add( ioobj->key(), false );
    fillListBox();
    listfld_->setCurrentItem( ioobj->name().buf() );
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
    ctio_.setObj( ret ? ret->clone() : nullptr );
    if ( isMultiChoice() && nrchosen>1 )
    {
	info.set( nrchosen ).add( "/" ).add( listfld_->size() )
			    .add( " chosen" );
    }
    else
    {
	if ( setnmfld && nmfld_ )
	    nmfld_->setText( ret ? ret->name().buf() : "" );
	info = getLimitedDisplayString( !ret ? "" :
		 ret->fullUserExpr(ctio_.ctxt_.forread_), 40, false, "..." );
    }

    triggerStatusMsg( info );
    return ret;
}


void uiIOObjSelGrp::triggerStatusMsg( const char* txt )
{
    CBCapsule<const char*> caps( txt, this );
    newStatusMsg.trigger( &caps );
}


void uiIOObjSelGrp::initGrpCB( CallBacker* cb )
{
    fullUpdate( -1 );
    setInitial( cb );
}


void uiIOObjSelGrp::setInitial( CallBacker* )
{
    if ( !ctio_.ctxt_.forread_ )
    {
	PtrMan<IOObj> ioobj = nullptr;
	if ( ctio_.ioobj_ )
	    nmfld_->setText( ctio_.ioobj_->name() );

	StringView presetnm = nmfld_->text();
	if ( !presetnm.isEmpty() && listfld_->isPresent(presetnm) )
	{
	    listfld_->setCurrentItem( presetnm );
	    if ( wrtrselfld_ )
	    {
		if ( !ioobj )
		    ioobj = IOM().get( currentID() );
		if ( ioobj )
		    wrtrselfld_->use( *ioobj );
	    }
	}
    }

    mAttachCB( listfld_->selectionChanged, uiIOObjSelGrp::selChg );
    mAttachCB( listfld_->itemChosen, uiIOObjSelGrp::choiceChg );
    mAttachCB( listfld_->deleteButtonPressed, uiIOObjSelGrp::delPress );
    if ( ctio_.ctxt_.forread_ )
    {
	selChg( nullptr );
	mAttachCB( IOM().entryAdded, uiIOObjSelGrp::objInserted );
    }

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
	    uiString tt = tr( "Set '%1' as default").arg(ioobj->uiName());
	    mkdefbut_->setToolTip( tt );
	}
	else if ( isdef )
	{
	    uiString deftt = tr("'%1' is default object").arg(ioobj->uiName());
	    mkdefbut_->setToolTip( deftt );
	}
	else
	    mkdefbut_->setToolTip( uiStrings::phrSetAs(uiStrings::sDefault()) );
    }

    if ( wrtrselfld_ && ioobj )
	wrtrselfld_->use( *ioobj );

    selectionChanged.trigger();
}


void uiIOObjSelGrp::choiceChg( CallBacker* )
{
    delete updStatusBarInfo( false );
    itemChosen.trigger();
}


void uiIOObjSelGrp::filtChg( CallBacker* )
{
    fullUpdate( 0 );
}


void uiIOObjSelGrp::objInserted( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, ky, cb );
    if ( !ky.isUdf() && ctio_.ctxt_.validObj(ky) )
	addEntry( ky );
}


void uiIOObjSelGrp::objRemoved( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, ky, cb );
    if ( !ky.isUdf())
	removeEntry( ky );
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
    if ( manipgrpsubj_ && manipgrpsubj_->manipgrp_ )
	manipgrpsubj_->manipgrp_->triggerButton( uiManipButGrp::Remove );
}


void uiIOObjSelGrp::makeDefaultCB(CallBacker*)
{
    PtrMan<IOObj> ioobj = IOM().get( currentID() );
    if ( !ioobj )
	return;

    ioobj->setSurveyDefault();

    const int cursel = currentItem();
    TypeSet<MultiID> chosenmids;
    getChosen( chosenmids );
    fullUpdate( 0 );
    setChosen( chosenmids );
    setCurrent( cursel );
}


void uiIOObjSelGrp::newOutputNameCB( CallBacker* )
{
}


void uiIOObjSelGrp::readChoiceDone( CallBacker* )
{
    if ( !lbchoiceio_ ) return;

    setChosen( lbchoiceio_->chosenKeys() );
}


void uiIOObjSelGrp::writeChoiceReq( CallBacker* )
{
    if ( !lbchoiceio_ )
	return;

    lbchoiceio_->keys().setEmpty();
    const TypeSet<MultiID>& mids = getIOObjIds();
    for ( int idx=0; idx< mids.size(); idx++ )
	lbchoiceio_->keys().add( mids[idx] );
}


void uiIOObjSelGrp::ctxtFileTypeChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,ctxtfiltfld,cb);
    if ( !ctxtfiltfld )
	return;

    const FileMultiString fms( ctxtfiltfld->name() );
    const BufferString withctxtfilter( fms[0] );
    BufferString filterval;
    if ( fms.size() > 1 )
	filterval.set( fms[1] );

    const int curitm = ctxtfiltfld->currentItem();
    if ( withctxtfilter == ctio_.ctxt_.trgroup_->groupName() )
    {
	if ( curitm <= 0 )
	    ctio_.ctxt_.toselect_.allowtransls_ = filterval.buf();
	else
	{
	    BufferString currnm = ctxtfiltfld->textOfItem( curitm );
	    if ( currnm.isEqual(dGBToDispStorageStr()) )
		currnm = mDGBKey;

	    ctio_.ctxt_.toselect_.allowtransls_ = currnm;
	}
    }

    fullUpdate( -2 );
}


void uiIOObjSelGrp::ctxtZDomainChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,ctxtfiltfld,cb);
    if ( !ctxtfiltfld )
	return;

    const FileMultiString fms( ctxtfiltfld->name() );
    const BufferString withctxtfilter( fms[0] );
    BufferString filterval;
    if ( fms.size() > 1 )
	filterval.set( fms[1] );

    const int curitm = ctxtfiltfld->currentItem();
    if ( curitm <= 0 && sKey::All()==ctxtfiltfld->textOfItem(0) )
    {
	if ( filterval.isEmpty() )
	    ctio_.ctxt_.toselect_.require_.removeWithKey( withctxtfilter );
	else
	    ctio_.ctxt_.require( withctxtfilter, filterval.str() );

	if ( withctxtfilter == ZDomain::sKey() )
	    ctio_.ctxt_.toselect_.require_.removeWithKey( ZDomain::sKeyUnit() );
    }
    else
    {
	const BufferString curzdom( ctxtfiltfld->textOfItem( curitm ));
	IOPar iop;
	iop.set( ZDomain::sKey(), curzdom );
	ctio_.ctxt_.require( withctxtfilter, curzdom.buf() );
    }

    fullUpdate( -2 );
    zDomainChanged.trigger();
}


void uiIOObjSelGrp::ctxtTypeChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,ctxtfiltfld,cb);
    if ( !ctxtfiltfld )
	return;

    const FileMultiString fms( ctxtfiltfld->name() );
    const BufferString withctxtfilter( fms[0] );
    BufferString filterval;
    if ( fms.size() > 1 )
	filterval.set( fms[1] );

    const int curitm = ctxtfiltfld->currentItem();
    if ( curitm <= 0 )
    {
	if ( filterval.isEmpty() )
	    ctio_.ctxt_.toselect_.require_.removeWithKey( withctxtfilter );
	else
	    ctio_.ctxt_.require( withctxtfilter, filterval.str() );
    }
    else
    {
	const BufferString curval( ctxtfiltfld->textOfItem( curitm ) );
	ctio_.ctxt_.require( withctxtfilter, curval.buf() );
    }

    fullUpdate( -2 );
}


const TypeSet<MultiID>& uiIOObjSelGrp::getIOObjIds() const
{
    return dataset_.getIOObjIds();
}


const TypeSet<MultiID>& uiIOObjSelGrp::getNeedsLocateIds() const
{
    return needslocateids_;
}
