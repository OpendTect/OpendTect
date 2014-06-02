/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjsel.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "linekey.h"
#include "transl.h"
#include "strmprov.h"
#include "survinfo.h"
#include "ioobjselectiontransl.h"
#include "od_iostream.h"
#include "ascstream.h"

#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "od_helpids.h"

#define mCtioObjTypeName(ctio) ctio.ctxt.objectTypeName()
#define mObjTypeName mCtioObjTypeName(ctio_)

static const MultiID udfmid( "-1" );


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


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
    , choicectio_(*mMkCtxtIOObj(IOObjSelection)) \
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
    nmfld_ = 0; manipgrpsubj = 0; mkdefbut_ = 0; asked2overwrite_ = false;
    if ( !ctio_.ctxt.forread )
	setup_.choicemode( OD::ChooseOnlyOne );
    choicectio_.ctxt.toselect.require_.add( sKey::Type(), mObjTypeName );

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
	listfld_->offerReadWriteSelection( mCB(this,uiIOObjSelGrp,readChoice),
					   mCB(this,uiIOObjSelGrp,writeChoice));

    fullUpdate( -1 );

    if ( ctio_.ioobj )
        listfld_->setCurrentItem( ctio_.ioobj->name() );

    if ( !ctio_.ctxt.forread )
    {
	nmfld_ = new uiGenInput( this, "Name" );
	nmfld_->attach( alignedBelow, topgrp_ );
	nmfld_->setElemSzPol( uiObject::SmallMax );
	nmfld_->setStretch( 2, 0 );

	LineKey lk( ctio_.name() );
	const BufferString nm( lk.lineName() );
	if ( !nm.isEmpty() )
	{
	    nmfld_->setText( nm );
	    const int listidx = listfld_->indexOf( nm );
	    if ( listidx < 0 )
		listfld_->setCurrentItem( 0 );
	    else
		listfld_->setChosen( listidx );
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
    delete choicectio_.ioobj;

    delete &ctio_;
    delete &choicectio_;
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
    if ( ioobj && ioobj->implExists(true) )
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

    ctio_.setObj( ioobj );
    ioobj.set( 0, false );
    return true;
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
    PtrMan<IOObj> ioobj = mkEntry( ctio_, seltxt );
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
    IOObj* ret = 0;

    if ( isMultiChoice() && nrchosen>1 )
    {
	info.set( nrchosen ).add( "/" ).add( listfld_->size() )
			    .add( " chosen" );
    }
    else
    {
	const int idx = listfld_->currentItem();
	ret = getIOObj( idx );
	ctio_.setObj( ret ? ret->clone() : 0 );
	if ( setnmfld && nmfld_ )
	    nmfld_->setText( ret ? ret->name() : "" );
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
    const char* presetnm = getNameField() ? getNameField()->text() : "";
    if ( *presetnm && !listfld_->isPresent(presetnm) )
    {
	if ( !listfld_->isPresent(presetnm) )
	    return;

	listfld_->setCurrentItem( presetnm );
    }

    listfld_->selectionChanged.notify( mCB(this,uiIOObjSelGrp,selChg) );
    listfld_->itemChosen.notify( mCB(this,uiIOObjSelGrp,choiceChg) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiIOObjSelGrp,delPress) );

    selChg( 0 );
}


void uiIOObjSelGrp::selChg( CallBacker* cb )
{
    if ( listfld_->doingBurstChoosing() )
	return;

    IOObj* ioobj = updStatusBarInfo( true );
    if ( mkdefbut_ )
	mkdefbut_->setSensitive( ioobj && ioobj->implExists(true) );
    delete ioobj;

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
    if ( nrChosen()!=1 )
	return;

    PtrMan<IOObj> ioobj = IOM().get( chosenID() );
    if ( !ioobj )
	return;

    ioobj->setSurveyDefault( surveydefaultsubsel_.str() );

    fullUpdate( 0 );
}


void uiIOObjSelGrp::readChoice(CallBacker*)
{
    choicectio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, choicectio_ );
    if ( !dlg.go() ) return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    const BufferString fnm( ioobj->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	const BufferString msg( "Cannot open : ", fnm, " for read" );
	uiMSG().error( msg );
	return;
    }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType( mCtioObjTypeName(choicectio_) ) )
    {
	const BufferString msg( fnm, " has the wrong file type: ",
				astrm.fileType() );
	uiMSG().error( msg );
	return;
    }

    TypeSet<MultiID> mids;
    while ( !atEndOfSection(astrm.next()) )
	mids += MultiID( astrm.value() );
    setChosen( mids );
}


void uiIOObjSelGrp::writeChoice( CallBacker* )
{
    choicectio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, choicectio_ );
    if ( !dlg.go() ) return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    const BufferString fnm( ioobj->fullUserExpr(false) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot open : ", fnm, " for write" );
	uiMSG().error( msg );
	return;
    }

    ascostream astrm( strm );
    astrm.putHeader( mCtioObjTypeName(choicectio_) );
    for ( int idx=0; idx<ioobjids_.size(); idx++ )
    {
	if ( isChosen(idx) )
	    astrm.put( ioobjnms_.get(idx), ioobjids_[idx]->buf() );
    }
    astrm.newParagraph();
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const uiString& seltxt, bool multisel,
			      bool havesetsurvdefault )
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread?"Input selection":"Output selection",
			mNoDlgTitle, mODHelpKey(mIOObjSelDlgHelpID) )
		.nrstatusflds(1))
	, selgrp_( 0 )
{
    uiIOObjSelGrp::Setup sgsu( c.ctxt.forread && multisel
			? OD::ChooseAtLeastOne : OD::ChooseOnlyOne );
    sgsu.allowsetdefault( havesetsurvdefault );
    selgrp_ = new uiIOObjSelGrp( this, c, sgsu );
    selgrp_->getListField()->setHSzPol( uiObject::WideVar );
    statusBar()->setTxtAlign( 0, Alignment::Right );
    selgrp_->newStatusMsg.notify( mCB(this,uiIOObjSelDlg,statusMsgCB));

    uiString titletext( seltxt );
    if ( titletext.isEmpty() )
    {
	if ( selgrp_->getContext().forread )
	    titletext = tr("Select input %1%2");
	else
	    titletext = tr("Select output %1%2");

	if ( selgrp_->getContext().name().isEmpty() )
	    titletext = titletext.arg( uiString(c.ctxt.trgroup->userName()) );
	else
	    titletext = titletext.arg( uiString( c.ctxt.name() ) );

	titletext = titletext.arg( multisel ? "(s)" : "" );
    }

    setTitleText( titletext );

    uiString captn( seltxt );
    if ( captn.isEmpty() )
    {
	if ( selgrp_->getContext().forread )
	{
	    if ( multisel )
		captn = tr( "Load %1(s)");
	    else
		captn = tr( "Load %1" );
	}
	else
	    captn = tr("Save %1 as" );

	if ( selgrp_->getContext().name().isEmpty() )
	    captn = captn.arg( uiString( c.ctxt.trgroup->userName() ) );
	else
	    captn = captn.arg( uiString( c.ctxt.name() ) );
    }
    setCaption( captn );

    setOkText( "&Ok (Select)" );
    selgrp_->getListField()->doubleClicked.notify(
	    mCB(this,uiDialog,accept) );
}


const IOObj* uiIOObjSelDlg::ioObj() const
{
    selgrp_->updateCtxtIOObj();
    return selgrp_->getCtxtIOObj().ioobj;
}


void uiIOObjSelDlg::statusMsgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( msg );
}


void uiIOObjSelDlg::setSurveyDefaultSubsel(const char* subsel)
{
    selgrp_->setSurveyDefaultSubsel(subsel);

}


#define mSelTxt(txt,ct) \
    txt ? txt \
        : (ct.name().isEmpty() ? ct.trgroup->userName().buf() : ct.name().buf())

uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c, const char* txt )
    : uiIOSelect(p,uiIOSelect::Setup(mSelTxt(txt,c)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(mSelTxt(txt,c))
    , inctiomine_(true)
{
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c,
			const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(true)
{
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt )
    : uiIOSelect(p,uiIOSelect::Setup(mSelTxt(txt,c.ctxt)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(mSelTxt(txt,c.ctxt))
    , inctiomine_(false)
{
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(false)
{
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


uiIOObjSel::~uiIOObjSel()
{
    if ( inctiomine_ )
	{ delete inctio_.ioobj; delete &inctio_; }
    delete workctio_.ioobj; delete &workctio_;
}


void uiIOObjSel::preFinaliseCB( CallBacker* )
{
    fillDefault();
    updateInput();
}


void uiIOObjSel::fillDefault()
{
    if ( setup_.filldef_ && !workctio_.ioobj && workctio_.ctxt.forread )
	workctio_.fillDefault();
}


void uiIOObjSel::setForRead( bool yn )
{
    inctio_.ctxt.forread = workctio_.ctxt.forread = yn;
    fillDefault();
}


bool uiIOObjSel::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::ID(),
	       workctio_.ioobj ? workctio_.ioobj->key() : MultiID() );
    return true;
}


bool uiIOObjSel::fillPar( IOPar& iopar, const char* bky ) const
{
    IOPar iop; fillPar( iop );
    iopar.mergeComp( iop, bky );
    return true;
}


void uiIOObjSel::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( sKey::ID() );
    if ( res && *res )
    {
	workctio_.setObj( MultiID(res) );
	setInput( MultiID(res) );
    }
}


void uiIOObjSel::usePar( const IOPar& iopar, const char* bky )
{
    if ( !bky || !*bky )
	usePar( iopar );
    else
    {
	IOPar* iop = iopar.subselect( bky );
	if ( iop )
	{
	    usePar( *iop );
	    delete iop;
	}
    }
}


void uiIOObjSel::setInput( const MultiID& mid )
{
    workctio_.setObj( IOM().get( mid ) );
    uiIOSelect::setInput( mid.buf() );
}


void uiIOObjSel::setInput( const IOObj& ioob )
{
    workctio_.setObj( ioob.clone() );
    uiIOSelect::setInput( ioob.key() );
}


void uiIOObjSel::updateInput()
{
    setInput( workctio_.ioobj ? workctio_.ioobj->key() : MultiID("") );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    mDeclStaticString( nm );
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky );
    return (const char*)nm;
}


void uiIOObjSel::obtainIOObj()
{
    LineKey lk( getInput() );
    const BufferString inp( lk.lineName() );
    if ( inp.isEmpty() )
	{ workctio_.setObj( 0 ); return; }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( ( inp == itemusrnm || lk == itemusrnm ) && workctio_.ioobj
			      && workctio_.ioobj->name()==inp.buf() )
	    return;
    }

    const IODir iodir( workctio_.ctxt.getSelKey() );
    const IOObj* ioob = iodir.get( inp.buf(),
				   workctio_.ctxt.trgroup->userName() );
    workctio_.setObj( ioob && workctio_.ctxt.validIOObj(*ioob)
		    ? ioob->clone() : 0 );
}


void uiIOObjSel::processInput()
{
    obtainIOObj();
    if ( workctio_.ioobj || workctio_.ctxt.forread )
	updateInput();
}


bool uiIOObjSel::existingUsrName( const char* nm ) const
{
    const IODir iodir ( workctio_.ctxt.getSelKey() );
    return iodir.get( nm, workctio_.ctxt.trgroup->userName() );
}


MultiID uiIOObjSel::validKey() const
{
    const IODir iodir( workctio_.ctxt.getSelKey() );
    const IOObj* ioob = iodir.get( getInput(),
				   workctio_.ctxt.trgroup->userName() );

    if ( ioob && workctio_.ctxt.validIOObj(*ioob) )
	return ioob->key();

    return MultiID();
}


void uiIOObjSel::doCommit( bool noerr ) const
{
    bool alreadyerr = noerr;
    const_cast<uiIOObjSel*>(this)->doCommitInput(alreadyerr);
    if ( !setup_.optional_ && !inctio_.ioobj && !alreadyerr )
    {
	uiString txt( inctio_.ctxt.forread
			 ? tr( "Please select the %1")
			 : tr( "Please enter a valid name for the %1" ) );
	uiMSG().error( txt.arg( mCtioObjTypeName(workctio_) ) );
    }
}


MultiID uiIOObjSel::key( bool noerr ) const
{
    doCommit(noerr);
    return inctio_.ioobj ? inctio_.ioobj->key() : MultiID("");
}


const IOObj* uiIOObjSel::ioobj( bool noerr ) const
{
    doCommit( noerr );
    return inctio_.ioobj;
}


IOObj* uiIOObjSel::getIOObj( bool noerr )
{
    doCommit( noerr );
    IOObj* ret = inctio_.ioobj; inctio_.ioobj = 0;
    return ret;
}


bool uiIOObjSel::commitInput()
{
    bool dum = false; return doCommitInput( dum );
}


#define mErrRet(s) \
{ if ( s && !alreadyerr ) uiMSG().error(s); alreadyerr = true; return false; }


bool uiIOObjSel::doCommitInput( bool& alreadyerr )
{
    LineKey lk( getInput() );
    const BufferString inp( lk.lineName() );
    if ( inp.isEmpty() )
    {
	if ( !haveempty_ )
	    return false;
	workctio_.setObj( 0 ); inctio_.setObj( 0 );
	commitSucceeded();
	return true;
    }

    processInput();
    if ( existingTyped() )
    {
	if ( workctio_.ioobj )
	{
	    const bool isalreadyok = inctio_.ioobj
			    && inctio_.ioobj->key() == workctio_.ioobj->key();
	    if ( !alreadyerr && !isalreadyok && !workctio_.ctxt.forread )
	    {
		const bool exists = workctio_.ioobj->implExists( false );
		if ( exists )
		{
		    if ( workctio_.ioobj->implReadOnly() )
			mErrRet(BufferString("'",getInput(),
					     "' exists and is read-only"))
		    if ( setup_.confirmoverwr_ && !uiMSG().askGoOn(
				BufferString("'",getInput(),
					     "' already exists. Overwrite?")) )
			mErrRet(0)
		}
	    }

	    inctio_.setObj( workctio_.ioobj->clone() );
	    commitSucceeded(); return true;
	}

	mErrRet(BufferString("'",getInput(),
		    "' already exists as another object type."
		    "\nPlease enter another name.") )
    }
    if ( workctio_.ctxt.forread ) return false;

    workctio_.setObj( createEntry( getInput() ) );
    inctio_.setObj( workctio_.ioobj ? workctio_.ioobj->clone() : 0 );
    if ( !inctio_.ioobj ) return false;

    commitSucceeded();
    return true;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    if ( !workctio_.ctxt.forread )
	workctio_.setName( getInput() );
    uiIOObjRetDlg* dlg = mkDlg();
    if ( !dlg ) return;
    uiIOObjSelGrp* selgrp_ = dlg->selGrp();
    if ( selgrp_ )
	selgrp_->setConfirmOverwrite( false ); // handle that here

    if ( !helpkey_.isEmpty() )
	dlg->setHelpKey( helpkey_ );
    if ( dlg->go() && dlg->ioObj() )
    {
	workctio_.setObj( dlg->ioObj()->clone() );
	updateInput();
	newSelection( dlg );
	selok_ = true;
    }

    delete dlg;
}


void uiIOObjSel::objSel()
{
    const char* ky = getKey();
    if ( !ky || !*ky )
	workctio_.setObj( 0 );
    else
	workctio_.setObj( IOM().get(ky) );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    return new uiIOObjSelDlg( this, workctio_, setup_.seltxt_ );
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    return mkEntry( workctio_, nm );
}
