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
#include "errh.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "linekey.h"
#include "transl.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistatusbar.h"


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
    selgrp_->selectionChg.notify( mCB(this,uiIOObjSelGrpManipSubj,selChg) );
}

const MultiID* curID() const
{
    int selidx = selgrp_->listfld_->currentItem();
    if ( selidx >= selgrp_->ioobjids_.size() )
	selidx = selgrp_->ioobjids_.size() - 1;
    return selidx < 0 ? 0 : selgrp_->ioobjids_[selidx];
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
    selgrp_->toStatusBar( msg );
}

    uiIOObjSelGrp*	selgrp_;
    uiIOObjManipGroup*	manipgrp_;

};



uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel,
			      bool havereloc, bool havesetsurvdefault )
    : uiGroup(p)
    , ctio_(c)
    , ismultisel_(multisel && ctio_.ctxt.forread)
    , nmfld_(0)
    , manipgrpsubj(0)
    , newStatusMsg(this)
    , mkdefbut_( 0 )
    , selectionChg(this)
    , confirmoverwrite_(true)
    , asked2overwrite_(false)
{
    IOM().to( ctio_.ctxt.getSelKey() );

    topgrp_ = new uiGroup( this, "Top group" );
    filtfld_ = new uiGenInput( topgrp_, "Filter", "*" );
    filtfld_->valuechanged.notify( mCB(this,uiIOObjSelGrp,filtChg) );
    if ( !seltxt || !*seltxt )
    {
	listfld_ = new uiListBox( topgrp_ );
	filtfld_->attach( centeredAbove, listfld_ );
	topgrp_->setHAlignObj( listfld_ );
    }
    else
    {
	uiLabeledListBox* llb = new uiLabeledListBox( topgrp_, seltxt );
	llb->attach( alignedBelow, filtfld_ );
	topgrp_->setHAlignObj( llb );
	listfld_ = llb->box();
    }

    listfld_->setName( "Objects list" );
    if ( ismultisel_ )
	listfld_->setMultiSelect( true );
    listfld_->setPrefHeightInChar( 8 );
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
	    if ( listfld_->isPresent( nm ) )
		listfld_->setCurrentItem( nm );
	    else
		listfld_->clearSelection();
	}
    }

    if ( !ismultisel_ && ctio_.ctxt.maydooper )
    {
	manipgrpsubj = new uiIOObjSelGrpManipSubj( this );
	manipgrpsubj->manipgrp_ = new uiIOObjManipGroup( *manipgrpsubj,
							 havereloc );
	
	if ( havesetsurvdefault )
	{
	    mkdefbut_ = manipgrpsubj->manipgrp_->addButton(
		"makedefault", "Set as default",
		mCB(this,uiIOObjSelGrp,makeDefaultCB) );
    }
    }

    listfld_->setHSzPol( uiObject::Wide );
    listfld_->selectionChanged.notify( mCB(this,uiIOObjSelGrp,selChg) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiIOObjSelGrp,delPress) );
    if ( (nmfld_ && !*nmfld_->text()) || !nmfld_ )
	selChg( this );
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
}


void uiIOObjSelGrp::setInitial( CallBacker* )
{
    const char* presetnm = getNameField() ? getNameField()->text() : "";
    if ( *presetnm )
    {
	if ( !getListField()->isPresent( presetnm ) )
	    return;
	else
	    getListField()->setCurrentItem( presetnm );
    }

    selChg( 0 );
}


int uiIOObjSelGrp::nrSel() const
{
    int nr = 0;
    for ( int idx=0; idx<listfld_->size(); idx++ )
	if ( listfld_->isSelected(idx) ) nr++;

    return nr;
}


uiIOObjManipGroup* uiIOObjSelGrp::getManipGroup()
{
    return manipgrpsubj ? manipgrpsubj->grp_ : 0;
}


const MultiID& uiIOObjSelGrp::selected( int objnr ) const
{
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isSelected(idx) )
	    objnr--;
	if ( objnr < 0 )
	    return *ioobjids_[idx];
    }

    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrsel="; msg += nrSel();
    pErrMsg( msg );
    return udfmid;
}


void uiIOObjSelGrp::setSelected( const TypeSet<MultiID>& mids )
{
    listfld_->selectAll( false );
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const int selidx = indexOf( ioobjids_, mids[idx] );
	listfld_->setSelected( selidx, true );
    }
}


void uiIOObjSelGrp::getSelected( TypeSet<MultiID>& mids ) const
{
    const int nrsel = nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	mids += selected( idx );
}


void uiIOObjSelGrp::delPress( CallBacker* )
{
    if ( manipgrpsubj && manipgrpsubj->manipgrp_ )
	manipgrpsubj->manipgrp_->triggerButton( uiManipButGrp::Remove );
}


void uiIOObjSelGrp::setSurveyDefaultSubsel(const char* subsel)
{
    surveydefaultsubsel_ = subsel;
}


void uiIOObjSelGrp::makeDefaultCB(CallBacker*)
{
    if ( nrSel()!=1 )
	return;
    
    PtrMan<IOObj> ioobj = IOM().get( selected() );
    if ( !ioobj )
	return;
    
    ioobj->setSurveyDefault( surveydefaultsubsel_.str() );
    
    fullUpdate( 0 );
}


void uiIOObjSelGrp::filtChg( CallBacker* )
{
    fullUpdate( 0 );
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
	    setCur( selidx );
    }
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );
    BufferString nmflt = filtfld_->text();
    if ( !nmflt.isEmpty() && nmflt != "*" )
	del.fill( IOM().dirPtr(), nmflt );

    ioobjnms_.erase();
    dispnms_.erase();
    deepErase( ioobjids_ );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( !ioobj )
	{
	    BufferString nm = del[idx]->name();
	    char* ptr = nm.buf();
	    mSkipBlanks( ptr );
	    dispnms_.add( ptr );
	    ioobjnms_.add( ptr );
	    ioobjids_ += new MultiID( udfmid );
	}
	else
	{
	    const MultiID ky( del[idx]->ioobj->key() );
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
    setCur( curidx );
}


void uiIOObjSelGrp::setCur( int curidx )
{
    if ( curidx >= ioobjnms_.size() )
	curidx = ioobjnms_.size() - 1;
    else if ( curidx < 0 )
	curidx = 0;
    if ( !ioobjnms_.isEmpty() )
	listfld_->setCurrentItem( curidx );
    selectionChg.trigger();
}


void uiIOObjSelGrp::fillListBox()
{
    listfld_->setEmpty();
    listfld_->addItems( dispnms_ );
}


void uiIOObjSelGrp::toStatusBar( const char* txt )
{
    CBCapsule<const char*> caps( txt, this );
    newStatusMsg.trigger( &caps );
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    bool issel = listfld_->isSelected( idx );
    if ( idx < 0 || !issel ) return 0;

    const MultiID& ky = *ioobjids_[idx];
    return IOM().get( ky );
}


void uiIOObjSelGrp::selChg( CallBacker* cb )
{
    BufferString info;
    bool allowsetdefault;
    if ( ismultisel_ && nrSel()>1 )
    {
	info += "Multiple objects selected (";
	info += nrSel(); info += "/"; info += listfld_->size(); info += ")";
	allowsetdefault = false;
    }
    else
    {
	PtrMan<IOObj> ioobj = getIOObj( listfld_->currentItem() );
	ctio_.setObj( ioobj ? ioobj->clone() : 0 );
	if ( cb && nmfld_ )
	    nmfld_->setText( ioobj ? ioobj->name() : "" );
	info = getLimitedDisplayString( !ioobj ? "" :
			 ioobj->fullUserExpr(ctio_.ctxt.forread), 40, false );
	allowsetdefault = ioobj && ioobj->implExists(true);
    }

    if ( mkdefbut_ )
	mkdefbut_->setSensitive( allowsetdefault );

    toStatusBar( info );
    selectionChg.trigger();
}


void uiIOObjSelGrp::setContext( const IOObjContext& c )
{
    ctio_.ctxt = c; ctio_.setObj( 0 );
    fullUpdate( -1 );
}


bool uiIOObjSelGrp::processInput()
{
    int curitm = listfld_->currentItem();
    if ( !nmfld_ )
    {
	// all 'forread' is handled here
	if ( ismultisel_ )
	{
	    if ( nrSel() > 0 )
		return true;
	    uiMSG().error( "Please select at least one item ",
		           "or press Cancel" );
	    return false;
	}

	PtrMan<IOObj> ioobj = getIOObj( listfld_->currentItem() );
	if ( !ioobj )
	{
	    IOM().to( "" );
	    fullUpdate( -1 );
	    return false;
	}
	ctio_.setObj( ioobj->clone() );
	return true;
    }

    // for write here
    LineKey lk( nmfld_->text() );
    const BufferString seltxt( lk.lineName() );
    int itmidx = ioobjnms_.indexOf( seltxt.buf() );
    if ( itmidx < 0 )
	return createEntry( seltxt );

    if ( itmidx != curitm )
	setCur( itmidx );
    PtrMan<IOObj> ioobj = getIOObj( itmidx );
    if ( ioobj && ioobj->implExists(true) )
    {
	bool ret = true;
	if ( ioobj->implReadOnly() )
	{
	    uiMSG().error( "Object is read-only" );
	    ret = false;
	}
	else if ( confirmoverwrite_ && !asked2overwrite_ )
	    ret = uiMSG().askOverwrite( "Overwrite existing object?" );

	if ( !ret )
	{
	    asked2overwrite_ = false;
	    return false;
	}

	asked2overwrite_ = true;
    }

    ctio_.setObj( ioobj );
    ioobj.set( 0, false );
    return true;
}


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    PtrMan<IOObj> ioobj = mkEntry( ctio_, seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
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
    selectionChg.trigger();
    return true;
}


bool uiIOObjSelGrp::fillPar( IOPar& iop ) const
{
    if ( !const_cast<uiIOObjSelGrp*>(this)->processInput() || !ctio_.ioobj )
	return false;

    if ( !ismultisel_ )
	iop.set( sKey::ID(), ctio_.ioobj->key() );
    else
    {
	TypeSet<MultiID> mids; getSelected( mids );
	iop.set( sKey::Size(), mids.size() );
	for ( int idx=0; idx<mids.size(); idx++ )
	    iop.set( IOPar::compKey(sKey::ID(),idx), mids[idx] );
    }

    return true;
}


void uiIOObjSelGrp::usePar( const IOPar& iop )
{
    if ( !ismultisel_ )
    {
	const char* res = iop.find( sKey::ID() );
	if ( !res || !*res ) return;

	const int selidx = indexOf( ioobjids_, MultiID(res) );
	if ( selidx >= 0 )
	    setCur( selidx );
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

	setSelected( mids );
    }
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel,
			      bool havesetsurvdefault)
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread?"Input selection":"Output selection",
		    	mNoDlgTitle,"8.1.1")
		.nrstatusflds(1))
	, selgrp_( 0 )
{
    const bool ismultisel = multisel && c.ctxt.forread;
    selgrp_ = new uiIOObjSelGrp( this, c, 0, multisel, false,
				 havesetsurvdefault );
    selgrp_->getListField()->setHSzPol( uiObject::Wide );
    statusBar()->setTxtAlign( 0, Alignment::Right );
    selgrp_->newStatusMsg.notify( mCB(this,uiIOObjSelDlg,statusMsgCB));

    BufferString nm( seltxt );
    if ( nm.isEmpty() )
    {
	nm = "Select ";
	nm += c.ctxt.forread ? "input " : "output ";
	if ( c.ctxt.name().isEmpty() )
	    nm += c.ctxt.trgroup->userName();
	else
	    nm += c.ctxt.name();
	if ( ismultisel ) nm += "(s)";
    }
    setTitleText( nm );

    BufferString captn( seltxt );
    if ( captn.isEmpty() )
    {
	captn = c.ctxt.forread ? "Load " : "Save ";
	if ( c.ctxt.name().isEmpty() )
	    captn += c.ctxt.trgroup->userName();
	else
	    captn += c.ctxt.name();
	if ( !c.ctxt.forread ) captn += " as";
	else if ( ismultisel ) captn += "(s)";
    }
    setCaption( captn );

    setOkText( "&Ok (Select)" );
    selgrp_->getListField()->doubleClicked.notify(
	    mCB(this,uiDialog,accept) );
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
    fillDefault();
    updateInput();
}


uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c,
			const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(true)
{
    fillDefault();
    updateInput();
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt )
    : uiIOSelect(p,uiIOSelect::Setup(mSelTxt(txt,c.ctxt)),
	    	 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(mSelTxt(txt,c.ctxt))
    , inctiomine_(false)
{
    fillDefault();
    updateInput();
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(false)
{
    fillDefault();
    updateInput();
}


uiIOObjSel::~uiIOObjSel()
{
    if ( inctiomine_ )
	{ delete inctio_.ioobj; delete &inctio_; }
    delete workctio_.ioobj; delete &workctio_;
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
    uiIOSelect::setInput( mid.buf() );
}


void uiIOObjSel::setInput( const IOObj& ioob )
{
    setInput( ioob.key() );
}


void uiIOObjSel::updateInput()
{
    setInput( workctio_.ioobj ? workctio_.ioobj->key() : MultiID("") );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    static BufferString nm;
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky );
    return (const char*)nm;
}


void uiIOObjSel::obtainIOObj()
{
    LineKey lk( getInput() );
    const BufferString inp( lk.lineName() );
    if ( specialitems.findKeyFor(inp) )
	{ workctio_.setObj( 0 ); return; }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( ( inp == itemusrnm || lk == itemusrnm ) && workctio_.ioobj 
			      && workctio_.ioobj->name()==inp.buf() )
	    return;
    }

    IOM().to( workctio_.ctxt.getSelKey() );
    const IOObj* ioob = (*IOM().dirPtr())[ inp.buf() ];
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
    IOM().to( workctio_.ctxt.getSelKey() );
    return (*IOM().dirPtr())[nm];
}


MultiID uiIOObjSel::validKey() const
{
    IOM().to( workctio_.ctxt.getSelKey() );
    const IOObj* ioob = (*IOM().dirPtr())[ getInput() ];

    if ( ioob && workctio_.ctxt.validIOObj(*ioob) )
	return ioob->key();

    return MultiID();
}


#define mDoCommit() \
    bool alreadyerr = noerr; \
    const_cast<uiIOObjSel*>(this)->doCommitInput(alreadyerr); \
    if ( !setup_.optional_ && !inctio_.ioobj && !alreadyerr ) \
    { \
	BufferString txt( inctio_.ctxt.forread \
				    ? "Please select the " \
				    : "Please enter a valid name for the " ); \
	txt += setup_.seltxt_; \
	uiMSG().error( txt ); \
    }


MultiID uiIOObjSel::key( bool noerr ) const
{
    mDoCommit();
    return inctio_.ioobj ? inctio_.ioobj->key() : MultiID("");
}


const IOObj* uiIOObjSel::ioobj( bool noerr ) const
{
    mDoCommit();
    return inctio_.ioobj;
}


IOObj* uiIOObjSel::getIOObj( bool noerr )
{
    mDoCommit();
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
    if ( specialitems.findKeyFor(inp) )
    {
	workctio_.setObj( 0 ); inctio_.setObj( 0 );
	commitSucceeded();
	return true;
    }
    if ( inp.isEmpty() )
	return false;

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
    if ( selgrp_ ) selgrp_->setConfirmOverwrite( false ); // handle that here

    if ( !helpid_.isEmpty() )
	dlg->setHelpID( helpid_ );
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
    if ( specialitems.find(ky) )
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
