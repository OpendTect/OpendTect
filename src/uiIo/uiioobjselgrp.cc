/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjselgrp.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "linekey.h"
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
#include "od_helpids.h"

#define mObjTypeName ctio_.ctxt.objectTypeName()

static const MultiID udfmid( "-1" );



class uiIOObjSelGrpManipSubj : public uiIOObjManipGroupSubj
{
public:

uiIOObjSelGrpManipSubj( uiIOObjSelGrp* sg )
    : uiIOObjManipGroupSubj(sg->listfld_)
    , selgrp_(sg)
    , manipgrp_(0)
{
    selgrp_->selectionChanged.notify( mCB(this,uiIOObjSelGrpManipSubj,selChg) );
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
    return selgrp_->ctio_.ctxt.trgroup->defExtension();
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
    ctio_.ctxt.fillTrGroup();
    nmfld_ = 0; wrtrselfld_ = 0;
    manipgrpsubj = 0; mkdefbut_ = 0; asked2overwrite_ = false;
    if ( !ctio_.ctxt.forread )
	setup_.choicemode( OD::ChooseOnlyOne );

    IOM().to( ctio_.ctxt.getSelKey() );

    topgrp_ = new uiGroup( this, "Top group" );
    filtfld_ = new uiGenInput( topgrp_, "Filter", "*" );
    filtfld_->valuechanged.notify( mCB(this,uiIOObjSelGrp,filtChg) );
    uiObject* lfatt;
    if ( seltxt.isEmpty() )
	lfatt = listfld_ = new uiListBox( topgrp_, "Objects",
					  setup_.choicemode_ );
    else
    {
	uiLabeledListBox* llb = new uiLabeledListBox( topgrp_, seltxt,
							setup_.choicemode_ );
	listfld_ = llb->box();
	lfatt = llb->attachObj();
    }
    lfatt->attach( centeredBelow, filtfld_ );
    topgrp_->setHAlignObj( lfatt );

    listfld_->setName( "Objects list" );
    listfld_->setPrefHeightInChar( 8 );
    listfld_->setHSzPol( uiObject::Wide );
    if ( isMultiChoice() )
    {
	lbchoiceio_ = new uiListBoxChoiceIO( *listfld_, mObjTypeName );
	lbchoiceio_->readDone.notify( mCB(this,uiIOObjSelGrp,readChoiceDone) );
	lbchoiceio_->storeRequested.notify(
				mCB(this,uiIOObjSelGrp,writeChoiceReq) );
    }

    fullUpdate( -1 );

    if ( ctio_.ioobj )
        listfld_->setCurrentItem( ctio_.ioobj->name() );

    if ( !ctio_.ctxt.forread )
    {
	uiGroup* wrgrp = new uiGroup( this, "Write group" );
	wrtrselfld_ = new uiIOObjSelWriteTranslator( wrgrp, ctio_, true );
	nmfld_ = new uiGenInput( wrgrp, uiStrings::sName() );
	nmfld_->setElemSzPol( uiObject::SmallMax );
	nmfld_->setStretch( 2, 0 );
	if ( !wrtrselfld_->isEmpty() )
	    nmfld_->attach( alignedBelow, wrtrselfld_ );
	wrgrp->setHAlignObj( nmfld_ );
	wrgrp->attach( alignedBelow, topgrp_ );

	LineKey lk( ctio_.name() );
	const BufferString nm( lk.lineName() );
	if ( !nm.isEmpty() )
	{
	    nmfld_->setText( nm );
	    const int listidx = listfld_->indexOf( nm );
	    if ( !ioobjids_.isEmpty() )
	    {
		int curitmidx = listidx < 0 ? 0 : listidx;
		listfld_->setCurrentItem( curitmidx );
	    }
	}
    }

    if ( ctio_.ctxt.maydooper )
    {
	manipgrpsubj = new uiIOObjSelGrpManipSubj( this );
	manipgrpsubj->manipgrp_ = new uiIOObjManipGroup( *manipgrpsubj,
							 setup_.allowreloc_,
							 setup_.allowremove_ );

	if ( setup_.allowsetdefault_ )
	{
	    mkdefbut_ = manipgrpsubj->manipgrp_->addButton(
		"makedefault", "Set as default",
		mCB(this,uiIOObjSelGrp,makeDefaultCB) );
	}
    }

    setHAlignObj( topgrp_ );
    postFinalise().notify( mCB(this,uiIOObjSelGrp,setInitial) );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    deepErase( ioobjids_ );
    if ( manipgrpsubj )
	delete manipgrpsubj->manipgrp_;
    delete manipgrpsubj;
    delete ctio_.ioobj;
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
    else if ( curidx < 0 )
	curidx = 0;
    if ( !ioobjnms_.isEmpty() )
    {
	listfld_->setCurrentItem( curidx );
	selectionChanged.trigger();
    }
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

    if ( isMultiChoice() )
	listfld_->chooseAll( false );

    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const int selidx = indexOf( ioobjids_, mids[idx] );
	if ( selidx >= 0 )
	    listfld_->setChosen( selidx, true );
    }
}


bool uiIOObjSelGrp::updateCtxtIOObj()
{
    const int curitm = listfld_->currentItem();
    const int sz = listfld_->size();
    if ( ctio_.ctxt.forread )
    {
	if ( isMultiChoice() )
	    return true;

	if ( curitm < 0 )
	{
	    ctio_.setObj( 0 );
	    if ( sz > 0 )
		uiMSG().error( "Please select the ", mObjTypeName,
			       "or press Cancel" );
	    return false;
	}
	PtrMan<IOObj> ioobj = getIOObj( curitm );
	if ( !ioobj )
	{
	    uiMSG().error( "Internal error: "
		"Cannot retrieve ", mObjTypeName, " details from data store" );
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
	if ( !wrtrselfld_->hasSelectedTranslator( *ioobj ) )
	{
	    uiMSG().error( "Sorry, can not change the storage type."
	       "\nIf you are sure, please remove the existing object first" );
	    return false;
	}

	if ( ioobj->implExists(true) )
	{
	    bool allok = true;
	    if ( ioobj->implReadOnly() )
	    {
		uiMSG().error( "Chosen ", mObjTypeName, " is read-only" );
		allok = false;
	    }
	    else if ( setup_.confirmoverwrite_ && !asked2overwrite_ )
		allok = uiMSG().askOverwrite(
			BufferString("Overwrite existing ",mObjTypeName,"?") );

	    if ( !allok )
		{ asked2overwrite_ = false; return false; }

	    asked2overwrite_ = true;
	}
    }

    ctio_.setObj( ioobj );
    ioobj.set( 0, false );

    if ( ctio_.ioobj )
    {
	wrtrselfld_->updatePars( *ctio_.ioobj );
	IOM().commitChanges( *ctio_.ioobj );
    }

    return true;
}


void uiIOObjSelGrp::setDefTranslator( const Translator* trl )
{
    if ( trl && wrtrselfld_ && *nmfld_->text() )
	wrtrselfld_->setTranslator( trl );
}


void uiIOObjSelGrp::setContext( const IOObjContext& c )
{
    ctio_.ctxt = c; ctio_.setObj( 0 );
    fullUpdate( -1 );
}


const IOObjContext& uiIOObjSelGrp::getContext() const
{
    return ctio_.ctxt;
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
    if ( !const_cast<uiIOObjSelGrp*>(this)->updateCtxtIOObj() || !ctio_.ioobj )
	return false;

    if ( !isMultiChoice() )
	iop.set( sKey::ID(), ctio_.ioobj->key() );
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
    const IODir iodir ( ctio_.ctxt.getSelKey() );
    IODirEntryList del( iodir, ctio_.ctxt );
    BufferString nmflt = filtfld_->text();
    if ( !nmflt.isEmpty() && nmflt != "*" )
	del.fill( iodir, nmflt );

    ioobjnms_.erase();
    dispnms_.erase();
    deepErase( ioobjids_ );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj )
	{
	    BufferString nm = del[idx]->name();
	    nm.trimBlanks();
	    dispnms_.add( nm );
	    ioobjnms_.add( nm );
	    ioobjids_ += new MultiID( udfmid );
	}
	else
	{
	    const MultiID ky( del[idx]->ioobj_->key() );
	    ioobjids_ += new MultiID( ky );
	    ioobjnms_.add( ioobj->name() );
	    BufferString dispnm;
	    const bool isdef = IOObj::isSurveyDefault(ky);
	    const bool ispl = StreamProvider::isPreLoaded(ky.buf(),true);
	    if ( isdef )
		dispnm += "> ";
	    if ( ispl )
		dispnm += "/ ";
	    dispnm += ioobj->name();
	    if ( ispl )
		dispnm += " \\";
	    if ( isdef )
		dispnm += " <";
	    dispnms_.add( dispnm );

	    if ( isdef && curidx < 0 )
		curidx = idx;
	}
    }

    fillListBox();
    setCurrent( curidx );
}


void uiIOObjSelGrp::fillListBox()
{
    NotifyStopper ns1( listfld_->selectionChanged );
    NotifyStopper ns2( listfld_->itemChosen );

    listfld_->setEmpty();
    listfld_->addItems( dispnms_ );

    selectionChanged.trigger();
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    return ioobjids_.validIdx(idx) ? IOM().get( *ioobjids_[idx] ) : 0;
}


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    PtrMan<IOObj> ioobj = wrtrselfld_->mkEntry( seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create ", mObjTypeName, " with this name" );
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
			 ret->fullUserExpr(ctio_.ctxt.forread), 40, false );
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
    if ( !ctio_.ctxt.forread )
    {
	PtrMan<IOObj> ioobj = 0;
	const char* presetnm = nmfld_->text();
	if ( !*presetnm && listfld_->size() == 1 )
	{
	    presetnm = listfld_->textOfItem( 0 );
	    ioobj = IOM().get( *ioobjids_[0] );
	    if ( ioobj )
		nmfld_->setText( BufferString(ioobjnms_.get(0)," 2") );
	}

	if ( *presetnm && listfld_->isPresent(presetnm) )
	    listfld_->setCurrentItem( presetnm );

	if ( wrtrselfld_ )
	{
	    if ( !ioobj )
		ioobj = IOM().get( currentID() );
	    if ( ioobj )
		wrtrselfld_->use( *ioobj );
	}
    }

    listfld_->selectionChanged.notify( mCB(this,uiIOObjSelGrp,selChg) );
    listfld_->itemChosen.notify( mCB(this,uiIOObjSelGrp,choiceChg) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiIOObjSelGrp,delPress) );

    if ( ctio_.ctxt.forread )
	selChg( 0 );
}


void uiIOObjSelGrp::selChg( CallBacker* cb )
{
    if ( listfld_->doingBurstChoosing() )
	return;

    PtrMan<IOObj> ioobj = updStatusBarInfo( true );
    if ( mkdefbut_ )
    {
	const bool enab = ioobj && ioobj->implExists(true);
	const bool isdef = ioobj && IOObj::isSurveyDefault( ioobj->key() );
	mkdefbut_->setSensitive( enab && !isdef );
	if ( enab && !isdef )
	{
	    BufferString tt( "Set '", ioobj->name(), "' as default" );
	    mkdefbut_->setToolTip( tt );
	}
	else
	    mkdefbut_->setToolTip( uiStrings::sEmptyString() );
    }

    if ( wrtrselfld_ && ioobj )
	wrtrselfld_->use( *ioobj );

    selectionChanged.trigger();
}


void uiIOObjSelGrp::choiceChg( CallBacker* cb )
{
    if ( !listfld_->doingBurstChoosing() )
	delete updStatusBarInfo( false );

    itemChosen.trigger();
}


void uiIOObjSelGrp::filtChg( CallBacker* )
{
    fullUpdate( 0 );
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
