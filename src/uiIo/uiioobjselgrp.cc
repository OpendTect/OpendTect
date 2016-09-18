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

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "strmprov.h"
#include "ioobjselectiontransl.h"
#include "od_iostream.h"
#include "ascstream.h"

#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistrings.h"
#include "settings.h"
#include "od_helpids.h"

#define mObjTypeName ctio_.ctxt_.objectTypeName()


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
    iconnms_.allowNull( true );
    ctio_.ctxt_.fillTrGroup();
    nmfld_ = 0; wrtrselfld_ = 0;
    manipgrpsubj = 0; mkdefbut_ = 0; asked2overwrite_ = false;
    if ( !ctio_.ctxt_.forread_ )
	setup_.choicemode( OD::ChooseOnlyOne );
    IOM().to( ctio_.ctxt_.getSelDirID() );

    mkTopFlds( seltxt );
    if ( !ctio_.ctxt_.forread_ )
	mkWriteFlds();
    mkManipulators();

    setHAlignObj( topgrp_ );
    postFinalise().notify( mCB(this,uiIOObjSelGrp,setInitial) );
}


void uiIOObjSelGrp::mkTopFlds( const uiString& seltxt )
{
    topgrp_ = new uiGroup( this, "Top group" );

    uiListBox::Setup su( setup_.choicemode_, seltxt );
    listfld_ = new uiListBox( topgrp_, su, "Objects" );

    filtfld_ = new uiGenInput( listfld_, uiStrings::sFilter(), "*" );
    filtfld_->updateRequested.notify( mCB(this,uiIOObjSelGrp,filtChg) );
    listfld_->box()->attach( centeredBelow, filtfld_ );

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
	wrtrselfld_ = new uiIOObjSelWriteTranslator( wrgrp, ctio_, true );
	wrtrselfld_->suggestedNameAvailble.notify(
				mCB(this,uiIOObjSelGrp,nameAvCB) );
    }

    nmfld_ = new uiGenInput( wrgrp, uiStrings::sName() );
    nmfld_->setElemSzPol( uiObject::SmallMax );
    nmfld_->setStretch( 2, 0 );
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

	inserter->objectInserted.notify( mCB(this,uiIOObjSelGrp,objInserted) );
	inserters_ += inserter;
    }

    insbutgrp->attach( centeredLeftOf, listfld_ );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
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
    if ( curidx >= ioobjnms_.size() )
	curidx = ioobjnms_.size() - 1;

    listfld_->setCurrentItem( curidx );
    selectionChanged.trigger();
}


void uiIOObjSelGrp::setCurrent( const DBKey& mid )
{
    const int idx = ioobjids_.indexOf( mid );
    if ( idx >= 0 )
	setCurrent( idx );
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
	    uiMSG().error(tr("Internal error: "
			     "Cannot retrieve %1 details from data store")
			.arg(mObjTypeName));
	    IOM().toRoot();
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
	if ( !res || !*res ) return;

	const int selidx = ioobjids_.indexOf( DBKey::getFromString(res) );
	if ( selidx >= 0 )
	    setCurrent( selidx );
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
    {
	selidx = ioobjids_.indexOf( ky );
	if ( selidx >= 0 )
	    setCurrent( selidx );
    }
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{

    mDefineStaticLocalObject( const bool, icsel_,
			      = Settings::common().isTrue("Ui.Icons.ObjSel") );
    ioobjnms_.setEmpty(); dispnms_.setEmpty(); iconnms_.setEmpty();
    ioobjids_.setEmpty();

    IODirEntryList entrylist( ctio_.ctxt_ );
    entrylist.fill( IODir(ctio_.ctxt_.getSelDirID()), filtfld_->text() );

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	const IOObj& ioobj = entrylist.ioobj( idx );
	const DBKey objid( ioobj.key() );

	if ( curidx < 0 )
	{
	    const bool issel = ctio_.ioobj_ && ctio_.ioobj_->key() == objid;
	    if ( issel )
		curidx = idx;
	    else if ( !ctio_.ioobj_ && ctio_.ctxt_.forread_ )
	    {
		if ( IOObj::isSurveyDefault(objid) )
		    curidx = idx;
	    }
	}

	ioobjids_.add( objid );
	ioobjnms_.add( ioobj.name() );
	dispnms_.add( entrylist.dispName(idx) );
	if ( icsel_ )
	    iconnms_.add( entrylist.iconName(idx) );
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
	BufferString icnm = iconnms_[idx];
	if ( icnm.isEmpty() )
	    icnm = "empty";
	listfld_->setIcon( idx, icnm );
    }

    selectionChanged.trigger();
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    return ioobjids_.validIdx(idx) ? IOM().get( ioobjids_[idx] ) : 0;
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
    ioobjids_.add( ioobj->key() );
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
    PtrMan<IOObj> ioobj = IOM().get( currentID() );
    if ( !ioobj )
	return;

    ioobj->setSurveyDefault( surveydefaultsubsel_.str() );

    const int cursel = currentItem();
    DBKeySet chosendbkys;
    getChosen( chosendbkys );
    fullUpdate( 0 );
    setChosen( chosendbkys );
    setCurrent( cursel );
}


void uiIOObjSelGrp::readChoiceDone( CallBacker* )
{
    if ( !lbchoiceio_ ) return;

    DBKeySet dbkys;
    for ( int idx=0; idx<lbchoiceio_->chosenKeys().size(); idx++ )
    {
	const BufferString kystr( lbchoiceio_->chosenKeys().get(idx) );
	if ( DBKey::isValidString(kystr) )
	    dbkys += DBKey::getFromString( kystr );
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
