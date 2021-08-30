/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		25/05/2000
________________________________________________________________________

-*/

#include "uiioobjselgrp.h"
#include "uiioobjinserter.h"
#include "uiioobjselwritetransl.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "hiddenparam.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "linekey.h"
#include "ioobjselectiontransl.h"
#include "mousecursor.h"
#include "od_iostream.h"
#include "strmprov.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistrings.h"
#include "settings.h"
#include "od_helpids.h"
#include "uilabel.h"

#define mObjTypeName ctio_.ctxt_.objectTypeName()

static const MultiID udfmid( "-1" );

static const char* dGBToDispStorageStr()    { return "OpendTect";  }
static HiddenParam<uiIOObjSelGrp,BufferStringSet*> trnotallowed_(nullptr);


class uiIOObjSelGrpManipSubj : public uiIOObjManipGroupSubj
{ mODTextTranslationClass(uiIOObjSelGrpManipSubj);
public:

uiIOObjSelGrpManipSubj( uiIOObjSelGrp* sg )
    : uiIOObjManipGroupSubj(sg->listfld_->box())
    , selgrp_(sg)
    , manipgrp_(0)
{
    selgrp_->selectionChanged.notify( mCB(this,uiIOObjSelGrpManipSubj,selChg) );
    selgrp_->itemChosen.notify( mCB(this,uiIOObjSelGrpManipSubj,selChg) );
}


MultiID currentID() const
{
    return selgrp_->currentID();
}


void getChosenIDs( TypeSet<MultiID>& mids ) const
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

};


#define muiIOObjSelGrpConstructorCommons \
      uiGroup(p) \
    , ctio_(*new CtxtIOObj(c)) \
    , lbchoiceio_(0) \
    , newStatusMsg(this) \
    , selectionChanged(this) \
    , itemChosen(this)


// Note: don't combine first and second constructor making the uiString default
// that will make things compile that shouldn't

uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
			      const uiString& seltxt )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
			      const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
		      const uiString& seltxt, const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const uiString& seltxt )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
		      const uiString& seltxt, const uiIOObjSelGrp::Setup& su )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, new BufferStringSet() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
					const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
		const uiString& seltxt, const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
	const uiIOObjSelGrp::Setup& su, const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const IOObjContext& c,
			const uiString& seltxt, const uiIOObjSelGrp::Setup& su,
			const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
					const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
		const uiString& seltxt, const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init( seltxt );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
	const uiIOObjSelGrp::Setup& su, const BufferStringSet& trnotallowed )
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init();
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
		      const uiString& seltxt, const uiIOObjSelGrp::Setup& su,
		    const BufferStringSet& trnotallowed)
    : muiIOObjSelGrpConstructorCommons
    , setup_(su)
{
    trnotallowed_.setParam( this, trnotallowed.clone() );
    init( seltxt );
}


void uiIOObjSelGrp::init( const uiString& seltxt )
{
    iconnms_.allowNull( true );
    ctio_.ctxt_.fillTrGroup();
    nmfld_ = 0; wrtrselfld_ = 0;
    manipgrpsubj = 0; mkdefbut_ = 0; asked2overwrite_ = false;
    if ( !ctio_.ctxt_.forread_ )
	setup_.choicemode( OD::ChooseOnlyOne );
    IOM().to( ctio_.ctxt_.getSelKey() );

    mkTopFlds( seltxt );
    if ( !ctio_.ctxt_.forread_ )
	mkWriteFlds();
    if ( ctio_.ctxt_.maydooper_ )
	mkManipulators();

    setHAlignObj( topgrp_ );
    postFinalise().notify( mCB(this,uiIOObjSelGrp,setInitial) );
}

uiObject* uiIOObjSelGrp::getFilterFieldAttachObj()
{
    return ctxtfiltfld_ ? ctxtfiltfld_ : filtfld_->attachObj();
}


void uiIOObjSelGrp::mkTopFlds( const uiString& seltxt )
{
    topgrp_ = new uiGroup( this, "Top group" );

    uiListBox::Setup su( setup_.choicemode_, seltxt );
    listfld_ = new uiListBox( topgrp_, su, "Objects" );

    filtfld_ = new uiGenInput( listfld_, uiStrings::sFilter(), "*" );
    filtfld_->setElemSzPol( uiObject::SmallVar );
    filtfld_->updateRequested.notify( mCB(this,uiIOObjSelGrp,filtChg) );
    const BufferString withctxtfilter( setup_.withctxtfilter_ );
    if ( !withctxtfilter.isEmpty() )
    {
	const IODir iodir( ctio_.ctxt_.getSelKey() );
	const IODirEntryList entrylist( iodir, ctio_.ctxt_ );
	BufferStringSet valstrs = entrylist.getValuesFor( withctxtfilter );
	const int idx = valstrs.indexOf( mDGBKey );
	if ( idx > -1 )
	    valstrs.get(idx).set( dGBToDispStorageStr() );
	//This is strictly for display purpose without changing the key
	if ( valstrs.size()>1 )
	{
	    valstrs.sort();
	    BufferString* firstline = new BufferString("All ");
	    firstline->add( withctxtfilter );
	    valstrs.insertAt( firstline, 0 );
	    auto* lbl = new uiLabel( listfld_, uiStrings::sType() );
	    ctxtfiltfld_ = new uiComboBox( listfld_, "ctxtfilter" );
	    ctxtfiltfld_->addItems( valstrs );
	    ctxtfiltfld_->attach( alignedBelow, filtfld_ );
	    lbl->attach( leftOf, ctxtfiltfld_ );
	    mAttachCB( ctxtfiltfld_->selectionChanged,
		       uiIOObjSelGrp::ctxtChgCB );
	}
    }
    listfld_->box()->attach( centeredBelow, getFilterFieldAttachObj() );
    topgrp_->setHAlignObj( listfld_ );

    listfld_->setName( "Objects list" );
    listfld_->box()->setPrefHeightInChar( 8 );
    listfld_->setHSzPol( uiObject::Wide );
    if ( isMultiChoice() )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_, mObjTypeName );
	lbchoiceio_->readDone.notify( mCB(this,uiIOObjSelGrp,readChoiceDone) );
	lbchoiceio_->storeRequested.notify(
				mCB(this,uiIOObjSelGrp,writeChoiceReq) );
    }

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
	wrtrselfld_ = new uiIOObjSelWriteTranslator( wrgrp, ctio_,
					getTrNotAllowed(), true );
	wrtrselfld_->suggestedNameAvailble.notify(
				mCB(this,uiIOObjSelGrp,nameAvCB) );
    }

    nmfld_ = new uiGenInput( wrgrp, uiStrings::sName() );
    mUseDefaultTextValidatorOnField(nmfld_);
    nmfld_->setElemSzPol( uiObject::SmallMax );
    nmfld_->setStretch( 2, 0 );
    mAttachCB( nmfld_->valuechanged, uiIOObjSelGrp::newOutputNameCB );
    if ( wrtrselfld_ && !wrtrselfld_->isEmpty() )
	nmfld_->attach( alignedBelow, wrtrselfld_ );
    wrgrp->setHAlignObj( nmfld_ );
    wrgrp->attach( alignedBelow, topgrp_ );

    LineKey lk( ctio_.name() );
    const BufferString nm( lk.lineName() );
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
	const BufferString trnm = tpls[idx]->typeName();
	const BufferStringSet& trnotallowed = getTrNotAllowed();
	if ( !IOObjSelConstraints::isAllowedTranslator(tpls[idx]->userName(),
	    ctio_.ctxt_.toselect_.allowtransls_) ||
		(!trnotallowed.isEmpty() && trnotallowed.indexOf(trnm)>=0) )
	    continue;

	uiIOObjInserter* inserter = uiIOObjInserter::create( *tpls[idx] );
	if ( !inserter || inserter->isDisabled() )
	    continue;

	inserter->setIOObjCtxt( ctio_.ctxt_ );
	uiToolButtonSetup* tbsu = inserter->getButtonSetup();
	if ( !tbsu )
	    { delete inserter; continue; }

	uiButton* but = tbsu->getButton( insbutgrp, true );
	delete tbsu;
	const int prevnrbuts = insertbuts_.size();
	insertbuts_ += but;
	if ( prevnrbuts > 0 )
	    but->attach( rightAlignedBelow, insertbuts_[prevnrbuts-1] );

	inserter->objectInserted.notify( mCB(this,uiIOObjSelGrp,objInserted) );
	inserters_ += inserter;
    }

    insbutgrp->attach( centeredLeftOf, listfld_ );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    detachAllNotifiers();
    deepErase( ioobjids_ );
    deepErase( inserters_ );
    if ( manipgrpsubj )
    {
	delete manipgrpsubj->manipgrp_;
	delete manipgrpsubj;
    }
    delete ctio_.ioobj_;
    delete &ctio_;
    delete lbchoiceio_;

    trnotallowed_.removeAndDeleteParam( this );
}


const BufferStringSet& uiIOObjSelGrp::getTrNotAllowed() const
{
    return *trnotallowed_.getParam(this);
}


void uiIOObjSelGrp::setTrNotAllowed( const BufferStringSet& trnotallowed )
{
    *trnotallowed_.getParam( this ) = trnotallowed;
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
    return ioobjids_.validIdx(selidx) ?  *ioobjids_[selidx] : MultiID("");
}


const MultiID& uiIOObjSelGrp::chosenID( int objnr ) const
{
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( isChosen(idx) )
	    objnr--;
	if ( objnr < 0 )
	    return *ioobjids_[idx];
    }

    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrChosen()="; msg += nrChosen();
    pErrMsg( msg );
    return udfmid;
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
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    nms.add( ioobjnms_.get( idx ) );
    }
}


void uiIOObjSelGrp::setCurrent( int curidx )
{
    if ( curidx >= ioobjnms_.size() )
	curidx = ioobjnms_.size() - 1;

    listfld_->setCurrentItem( curidx );
    selectionChanged.trigger();
}


void uiIOObjSelGrp::setCurrent( const MultiID& mid )
{
    const int idx = indexOf( ioobjids_, mid );
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
	const int selidx = indexOf( ioobjids_, mids[idx] );
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
		uiMSG().error(tr("Please select the %1 "
				 "or press Cancel").arg(mObjTypeName));
	    return false;
	}
	PtrMan<IOObj> ioobj = getIOObj( curitm );
	if ( !ioobj )
	{
	    uiMSG().error(tr("Internal error: "
			     "Cannot retrieve %1 details from data store")
			.arg(mObjTypeName));
	    IOM().to( MultiID(0) );
	    fullUpdate( -1 );
	    return false;
	}
	ctio_.setObj( ioobj->clone() );
	return true;
    }

    // from here for write only
    LineKey lk( nmfld_->text() );
    const BufferString seltxt( lk.lineName() );
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


void uiIOObjSelGrp::displayManipGroup( bool yn, bool shrink )
{
    if ( manipgrpsubj && manipgrpsubj->grp_ )
	manipgrpsubj->grp_->display( yn, shrink );
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
	const char* res = iop.find( sKey::ID() );
	if ( !res || !*res ) return;

	const int selidx = indexOf( ioobjids_, MultiID(res) );
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
    int selidx = indexOf( ioobjids_, ky );
    fullUpdate( selidx );
    // Maybe a new one has been added
    if ( selidx < 0 )
    {
	selidx = indexOf( ioobjids_, ky );
	if ( selidx >= 0 )
	    setCurrent( selidx );
    }
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    const IODir iodir ( ctio_.ctxt_.getSelKey() );
    IODirEntryList del( iodir, ctio_.ctxt_ );
    BufferString nmflt = filtfld_->text();
    if ( !nmflt.isEmpty() && nmflt != "*" )
	del.fill( iodir, nmflt );

    mDefineStaticLocalObject( const bool, icsel_,
			      = Settings::common().isTrue("Ui.Icons.ObjSel") );
    ioobjnms_.erase();
    dispnms_.erase();
    iconnms_.erase();
    deepErase( ioobjids_ );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;

	// 'uiIOObjEntryInfo'
	BufferString dispnm( del[idx]->name() );
	BufferString ioobjnm;
	MultiID objid( udfmid );
	const char* icnm = 0;

	if ( !ioobj )
	    ioobjnm = dispnm;
	else
	{
	    objid = del[idx]->ioobj_->key();
	    const bool issel = ctio_.ioobj_ && ctio_.ioobj_->key() == objid;
	    const bool isdef = setup_.allowsetdefault_
			? IOObj::isSurveyDefault( objid ) : false;
	    const bool ispl = StreamProvider::isPreLoaded( objid.buf(), true );

	    ioobjnm = ioobj->name();
	    dispnm.setEmpty();
	    if ( isdef ) dispnm += "> ";
	    if ( ispl ) dispnm += "/ ";
	    dispnm += ioobj->name();
	    if ( ispl ) dispnm += " \\";
	    if ( isdef ) dispnm += " <";

	    if ( curidx < 0 )
	    {
		if ( issel || (isdef && !ctio_.ioobj_ && ctio_.ctxt_.forread_) )
		curidx = idx;
	    }

	    if ( icsel_ )
	    {
		PtrMan<Translator> transl = ioobj->createTranslator();
		if ( transl )
		    icnm = transl->iconName();
	    }
	}

	//TODO cleaner is to put this into one object: uiIOObjEntryInfo
	ioobjids_ += new MultiID( objid );
	ioobjnms_.add( ioobjnm );
	dispnms_.add( dispnm );
	if ( icsel_ )
	    iconnms_ += icnm;
    }

    fillListBox();
    setCurrent( curidx );
    selChg( 0 );
}


void uiIOObjSelGrp::fillListBox()
{
    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );

    listfld_->setEmpty();
    listfld_->addItems( dispnms_ );
    for ( int idx=0; idx<iconnms_.size(); idx++ )
    {
	const char* icnm = iconnms_[idx];
	if ( !icnm ) icnm = "empty";
	listfld_->setIcon( idx, icnm );
    }

    selectionChanged.trigger();
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    return ioobjids_.validIdx(idx) ? IOM().get( *ioobjids_[idx] ) : 0;
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

    ioobjnms_.add( ioobj->name() );
    dispnms_.add( ioobj->name() );
    ioobjids_ += new MultiID( ioobj->key() );
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
	if ( setnmfld && nmfld_ )
	    nmfld_->setText( ret ? ret->name().buf() : "" );
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
		    ioobj = IOM().get( currentID() );
		if ( ioobj )
		    wrtrselfld_->use( *ioobj );
	    }
	}
    }

    listfld_->selectionChanged.notify( mCB(this,uiIOObjSelGrp,selChg) );
    listfld_->itemChosen.notify( mCB(this,uiIOObjSelGrp,choiceChg) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiIOObjSelGrp,delPress) );

    if ( ctio_.ctxt_.forread_ )
	selChg( 0 );
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
    mCBCapsuleUnpack( MultiID, ky, cb );
    if ( !ky.isEmpty() )
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
    PtrMan<IOObj> ioobj = IOM().get( currentID() );
    if ( !ioobj )
	return;

    ioobj->setSurveyDefault( surveydefaultsubsel_.str() );

    const int cursel = currentItem();
    TypeSet<MultiID> chosenmids;
    getChosen( chosenmids );
    fullUpdate( 0 );
    setChosen( chosenmids );
    setCurrent( cursel );
}


void uiIOObjSelGrp::newOutputNameCB( CallBacker* )
{
    const int deftransidx = ctio_.ctxt_.trgroup_->defTranslIdx();
    const Translator* deftrans = ctio_.ctxt_.trgroup_->templates()[deftransidx];
    PtrMan<IOObj> curioobj = IOM().get( currentID() );
    const Translator* selectedtrans =
				wrtrselfld_ ? wrtrselfld_->selectedTranslator()
					    : 0;
    const bool translatorchanged = curioobj && selectedtrans
			 ? curioobj->translator() != selectedtrans->userName()
			 : false;

    if ( !translatorchanged && wrtrselfld_ )
    {
	if ( curioobj && wrtrselfld_->hasWriteOpts() )
	{
	    uiIOObjSelWriteTranslator currentwrtselfld( this, ctio_.ctxt_,
					    getTrNotAllowed(), true );
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

    TypeSet<MultiID> mids;
    for ( int idx=0; idx<lbchoiceio_->chosenKeys().size(); idx++ )
	mids += MultiID( lbchoiceio_->chosenKeys().get(idx).buf() );

    setChosen( mids );
}


void uiIOObjSelGrp::writeChoiceReq( CallBacker* )
{
    if ( !lbchoiceio_ ) return;

    lbchoiceio_->keys().setEmpty();
    for ( int idx=0; idx<ioobjids_.size(); idx++ )
	lbchoiceio_->keys().add( ioobjids_[idx]->buf() );
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
		BufferString currnm = ctxtfiltfld_->textOfItem( curitm );
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
					ctxtfiltfld_->textOfItem( curitm ) );
	}
	fullUpdate( -2 );
    }
}
