/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uiioobjinserter.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "transl.h"

#include "uimsg.h"
#include "uistatusbar.h"
#include "uilistbox.h"


// uiIOObjRetDlg

uiIOObjRetDlg::uiIOObjRetDlg( uiParent* p, const Setup& s )
    : uiDialog(p,s)
{}


uiIOObjRetDlg::~uiIOObjRetDlg()
{}


// uiIOObjSelDlg

static HelpKey getHelpKey( bool forread )
{
    return forread ? mODHelpKey(mIOObjInputSelDlgHelpID)
		   : mODHelpKey(mIOObjOutputSelDlgHelpID);
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const IOObjContext& ctxt,
			      const Setup& setup )
    : uiIOObjRetDlg(p,uiDialog::Setup(selTxt(ctxt.forread_),
				      mNoDlgTitle,
				      getHelpKey(ctxt.forread_))
			.nrstatusflds(1))
    , setup_(setup)
{
    CtxtIOObj ctio( ctxt );
    init( ctio );
    ctio.destroyAll();
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& ctio,
				const uiString& ttxt )
    : uiIOObjRetDlg(p,uiDialog::Setup(selTxt(ctio.ctxt_.forread_),
				      mNoDlgTitle,
				      getHelpKey(ctio.ctxt_.forread_))
			.nrstatusflds(1))
    , setup_( ttxt )
{
    init( ctio );
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const uiIOObjSelDlg::Setup& su,
				const CtxtIOObj& ctio )
    : uiIOObjRetDlg(p,uiDialog::Setup(selTxt(ctio.ctxt_.forread_),
				      mNoDlgTitle,
				      getHelpKey(ctio.ctxt_.forread_))
			.nrstatusflds(1))
    , setup_( su )
{
    init( ctio );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    detachAllNotifiers();
}


uiString uiIOObjSelDlg::selTxt( bool forread )
{
    return forread ? uiStrings::sInputSelection()
		   : uiStrings::sOutputSelection();
}


bool uiIOObjSelDlg::isForRead()
{
    return selgrp_ ? selgrp_->getContext().forread_ : false;
}


void uiIOObjSelDlg::init( const CtxtIOObj& ctio )
{
    uiIOObjSelGrp::Setup sgsu( ctio.ctxt_.forread_ && setup_.multisel_
			? OD::ChooseAtLeastOne : OD::ChooseOnlyOne );
    sgsu.allowsetdefault( setup_.allowsetsurvdefault_ );
    sgsu.withwriteopts( setup_.withwriteopts_ );
    sgsu.withinserters( setup_.withinserters_ );
    sgsu.trsnotallwed( setup_.trsnotallwed_ );
    selgrp_ = new uiIOObjSelGrp( this, ctio, sgsu );
    selgrp_->getListField()->resizeToContents();
    statusBar()->setTxtAlign( 0, Alignment::Right );
    mAttachCB(selgrp_->newStatusMsg, uiIOObjSelDlg::statusMsgCB);

    int nr = setup_.multisel_ ? mPlural : 1;

    uiString titletext( setup_.titletext_ );
    if ( titletext.isEmpty() )
    {
	if ( selgrp_->getContext().forread_ )
	    titletext = uiStrings::phrSelect(uiStrings::sInput());
	else
	    titletext = uiStrings::phrSelect(uiStrings::sOutput());
	if ( selgrp_->getContext().name().isEmpty() )
	    titletext = toUiString("%1 %2").arg(titletext)
				    .arg( ctio.ctxt_.trgroup_->typeName(nr) );
	else
	    titletext = toUiString("%1 %2").arg(titletext)
				   .arg( toUiString( ctio.ctxt_.name() ) );
    }

    setTitleText( titletext );

    uiString captn;
    if ( !selgrp_->getContext().forread_ )
	captn = tr("Save %1 as" );
    else
	captn = tr( "Select %1" );

   if ( selgrp_->getContext().name().isEmpty() )
	captn = captn.arg( ctio.ctxt_.trgroup_->typeName(nr) );
    else
	captn = captn.arg( toUiString(ctio.ctxt_.name()) );
    setCaption( captn );

    mAttachCB(selgrp_->getListField()->doubleClicked, uiDialog::accept);
}


const IOObj* uiIOObjSelDlg::ioObj() const
{
    selgrp_->updateCtxtIOObj();
    return selgrp_->getCtxtIOObj().ioobj_;
}


void uiIOObjSelDlg::statusMsgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( mToUiStringTodo(msg) );
}


// uiIOObjSel::Setup

uiIOObjSel::Setup::Setup( const uiString& seltxt )
    : uiIOSelect::Setup(seltxt)
    , confirmoverwr_(true)
    , withinserters_(true)
    , withwriteopts_(true)
    , filldef_(true)
{}


uiIOObjSel::Setup::~Setup()
{}


// uiIOObjSel

static uiString getString( const uiString& txt, const IOObjContext& ct )
{
    if ( !txt.isEmpty() )
	return txt;

    return toUiString( ct.name().isEmpty() ? ct.trgroup_->groupName().buf()
					   : ct.name().buf() );
}


uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c, const uiString& txt)
    : uiIOSelect(p,uiIOSelect::Setup(getString(txt,c)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(getString(txt,c))
    , inctiomine_(true)
{
    init();
}


uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c,
			const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(true)
{
    init();
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiString& txt )
    : uiIOSelect(p,uiIOSelect::Setup(getString(txt,c.ctxt_)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(getString(txt,c.ctxt_))
    , inctiomine_(false)
{
    init();
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(false)
{
    init();
}


void uiIOObjSel::init()
{
    workctio_.ctxt_.fillTrGroup();
    wrtrselfld_ = nullptr;
    const bool insertersdisabled = uiIOObjInserter::allDisabled();
    if ( workctio_.ctxt_.forread_ && setup_.withinserters_
				  && !insertersdisabled )
    {
	uiButton* insertbut =
		uiIOObjInserter::createInsertButton( this, workctio_,
						     inserters_,
						     setup_.trsnotallwed_ );
	extselbuts_.add( insertbut );
	for ( auto* inserter : inserters_ )
	    mAttachCB( inserter->objInserterd, uiIOObjSel::objInserted );
    }

    if ( !workctio_.ctxt_.forread_ && setup_.withwriteopts_ )
    {
	wrtrselfld_ = new uiIOObjSelWriteTranslator( this, workctio_,
					    setup_.trsnotallwed_, false );
	wrtrselfld_->attach( rightOf, uiIOSelect::endObj(false) );
    }

    mAttachCB( preFinalize(), uiIOObjSel::preFinalizeCB );
    mAttachCB( IOM().afterSurveyChange, uiIOObjSel::survChangedCB );
    mAttachCB( optionalChecked, uiIOObjSel::optCheckCB );
}


uiIOObjSel::~uiIOObjSel()
{
    detachAllNotifiers();
    deepErase( inserters_ );
    if ( inctiomine_ )
    {
	delete inctio_.ioobj_;
	delete &inctio_;
    }

    delete workctio_.ioobj_; delete &workctio_;
}


void uiIOObjSel::optCheckCB( CallBacker* )
{
    if ( wrtrselfld_ )
	wrtrselfld_->setSensitive( isChecked() );
}


void uiIOObjSel::preFinalizeCB( CallBacker* )
{
    initRead();
}


void uiIOObjSel::survChangedCB( CallBacker* )
{
    if ( workctio_.ioobj_ )
	deleteAndNullPtr( workctio_.ioobj_ );

    initRead();
}


void uiIOObjSel::initRead()
{
    {
	NotifyStopper ns( selectionDone );
	fillEntries();
    }

    if( !workctio_.ioobj_ )
	fillDefault();
    updateInput();
}


uiObject* uiIOObjSel::endObj( bool left )
{
    if ( wrtrselfld_ && !left && !wrtrselfld_->isEmpty() )
	return wrtrselfld_->attachObj();
    return uiIOSelect::endObj( left );
}


void uiIOObjSel::fillDefault()
{
    if ( setup_.filldef_ && !workctio_.ioobj_ && workctio_.ctxt_.forread_ )
	workctio_.fillDefault();
}


void uiIOObjSel::fillEntries()
{
    if ( !inctio_.ctxt_.forread_ )
	return;

    const bool hadselioobj = workctio_.ioobj_;

    const IODir iodir( inctio_.ctxt_.getSelKey() );
    BufferStringSet keys, names;
    if ( setup_.withclear_ || !setup_.filldef_ )
    {
	keys.add( "" );
	names.add( "" );
    }

    const IODirEntryList entries( iodir, inctio_.ctxt_ );
    for ( const auto* entry : entries )
    {
	const IOObj* obj = entry->ioobj_;
	if ( !obj )
	    continue;

	keys.add( obj->key().toString() );
	names.add( obj->name().buf() );
    }

    setEntries( keys, names );
    if ( !hadselioobj && !keys.isEmpty() )
	workctio_.setObj( 0 );
}


IOObjContext uiIOObjSel::getWriteIOObjCtxt( IOObjContext ctxt )
{
    ctxt.forread_ = false;
    return ctxt;
}


void uiIOObjSel::require( const char* keystr, const char* typ, bool allowempty )
{
    workctio_.ctxt_.require( keystr, typ, allowempty );
}


void uiIOObjSel::requireType( const char* typ, bool allowempty )
{
    workctio_.ctxt_.requireType( typ, allowempty );
}


void uiIOObjSel::requireZDef( const ZDomain::Def& zdef, bool allowempty )
{
    workctio_.ctxt_.requireZDef( zdef, allowempty );
}


void uiIOObjSel::requireZDomain( const ZDomain::Info& zinfo, bool allowempty )
{
    workctio_.ctxt_.requireZDomain( zinfo, allowempty );
}


const ZDomain::Def* uiIOObjSel::requiredZDef() const
{
    return workctio_.ctxt_.requiredZDef();
}


const ZDomain::Info* uiIOObjSel::requiredZDomain() const
{
    return workctio_.ctxt_.requiredZDomain();
}


bool uiIOObjSel::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::ID(),
	       workctio_.ioobj_ ? workctio_.ioobj_->key() : MultiID() );
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
    MultiID mid;
    iopar.get( sKey::ID(), mid );
    if ( !mid.isUdf() )
    {
	workctio_.setObj( mid );
	setInput( mid );
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
    IOObj* inpobj = IOM().get( mid );
    workctio_.setObj( inpobj );
    uiIOSelect::setInput( inpobj ? mid.toString() : nullptr );
    if ( inpobj && wrtrselfld_ )
	wrtrselfld_->use( *inpobj );
}


void uiIOObjSel::setInput( const IOObj& ioobj )
{
    setInput( ioobj.key() );
}


void uiIOObjSel::updateInput()
{
    setInput( workctio_.ioobj_ ? workctio_.ioobj_->key() : MultiID::udf() );
}


void uiIOObjSel::setEmpty()
{
    uiIOSelect::setEmpty();
    workctio_.setObj( 0 ); inctio_.setObj( 0 );
    updateInput();
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
    StringPair input( getInput() );
    const BufferString& inp = input.first();
    if ( inp.isEmpty() )
    {
	workctio_.setObj( nullptr );
	return;
    }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( ( inp==itemusrnm || input.getCompString()==itemusrnm )
				&& workctio_.ioobj_
				&& workctio_.ioobj_->name()==inp.buf() )
	    return;
    }

    const IODir iodir( workctio_.ctxt_.getSelKey() );
    const IOObj* ioob = iodir.get( inp.buf(),
				   workctio_.ctxt_.trgroup_->groupName() );
    workctio_.setObj( ioob && workctio_.ctxt_.validIOObj(*ioob)
		    ? ioob->clone() : 0 );
}


void uiIOObjSel::processInput()
{
    BufferString tmpstr( getInput() );
    tmpstr.trimBlanks();
    setInputText( tmpstr.buf() );
    obtainIOObj();
    if ( workctio_.ioobj_ || workctio_.ctxt_.forread_ )
	updateInput();
}


bool uiIOObjSel::existingUsrName( const char* nm ) const
{
    const IODir iodir ( workctio_.ctxt_.getSelKey() );
    return iodir.get( nm, workctio_.ctxt_.trgroup_->groupName() );
}


MultiID uiIOObjSel::validKey() const
{
    const IODir iodir( workctio_.ctxt_.getSelKey() );
    const IOObj* ioob = iodir.get( getInput(),
				   workctio_.ctxt_.trgroup_->groupName() );

    if ( ioob && workctio_.ctxt_.validIOObj(*ioob) )
	return ioob->key();

    return MultiID();
}


void uiIOObjSel::doCommit( bool noerr ) const
{
    bool alreadyerr = noerr;
    const_cast<uiIOObjSel*>(this)->doCommitInput(alreadyerr);
}


MultiID uiIOObjSel::key( bool noerr ) const
{
    doCommit(noerr);
    return inctio_.ioobj_ ? inctio_.ioobj_->key() : MultiID::udf();
}


const IOObj* uiIOObjSel::ioobj( bool noerr ) const
{
    doCommit( noerr );
    return inctio_.ioobj_;
}


IOObj* uiIOObjSel::getIOObj( bool noerr )
{
    doCommit( noerr );
    IOObj* ret = inctio_.ioobj_; inctio_.ioobj_ = 0;
    return ret;
}


bool uiIOObjSel::commitInput()
{
    bool dum = false; return doCommitInput( dum );
}


#define mErrRet(s) \
{ if ( !s.isEmpty() && !alreadyerr ) uiMSG().error(s); alreadyerr = true;  \
return false; }


bool uiIOObjSel::doCommitInput( bool& alreadyerr )
{
    StringPair input( getInput() );
    const BufferString& inp = input.first();
    if ( inp.isEmpty() )
    {
	if ( !haveempty_ )
	{
	    if ( !setup_.optional_ && !alreadyerr )
	    {
		uiString txt( inctio_.ctxt_.forread_
				? tr( "Please select the %1")
				: tr( "Please enter a valid name for the %1" ));
		uiMSG().error( txt.arg(setup_.seltxt_) );
	    }

	    return false;
	}

	workctio_.setObj( 0 ); inctio_.setObj( 0 );
	commitSucceeded();
	return true;
    }

    processInput();
    if ( existingTyped() )
    {
	if ( workctio_.ioobj_ )
	{
	    if ( !workctio_.ctxt_.forread_ && wrtrselfld_
		&& !wrtrselfld_->isEmpty()
		&& !wrtrselfld_->hasSelectedTranslator(*workctio_.ioobj_) )
		mErrRet( tr("Cannot change the output format "
			 "for an already existing entry") )

	    const bool isalreadyok = inctio_.ioobj_
			    && inctio_.ioobj_->key() == workctio_.ioobj_->key();
	    if ( !alreadyerr && !isalreadyok && !workctio_.ctxt_.forread_ )
	    {
		const bool exists = workctio_.ioobj_->implExists( false );
		if ( exists )
		{
		    if ( workctio_.ioobj_->implReadOnly() )
			mErrRet(tr("'%1' exists and is read-only").arg(
				getInput()))
		    if ( setup_.confirmoverwr_ && !uiMSG().askGoOn(
				tr("'%1' already exists. Overwrite?")
				.arg(getInput())) )
			mErrRet(uiStrings::sEmptyString())
		}
	    }

	    inctio_.setObj( workctio_.ioobj_->clone() );
	    commitSucceeded(); return true;
	}

	mErrRet(tr("'%1' already exists as another object type."
	       "\nPlease enter another name.").arg(getInput()))

    }

    if ( workctio_.ctxt_.forread_ )
	return false;

    workctio_.setObj( createEntry( getInput() ) );
    inctio_.setObj( workctio_.ioobj_ ? workctio_.ioobj_->clone() : nullptr );
    if ( !inctio_.ioobj_ )
    {
	if ( !alreadyerr )
	    uiMSG().error(
		    uiStrings::phrCannotCreateDBEntryFor(setup_.seltxt_) );

	return false;
    }

    commitSucceeded();
    return true;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    if ( !workctio_.ctxt_.forread_ )
	workctio_.setName( getInput() );

    uiIOObjRetDlg* dlg = mkDlg();
    if ( !dlg )
	return;

    if ( dlg->go() && dlg->ioObj() )
    {
	workctio_.setObj( dlg->ioObj()->clone() );
	updateInput();
	newSelection( dlg );
	selok_ = true;
    }

    fillEntries();
    delete dlg;
}


void uiIOObjSel::objInserted( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, ky, cb );
    if ( !ky.isUdf() )
	setInput( ky );
}


void uiIOObjSel::objSel()
{
    MultiID ky;
    ky.fromString( getKey() );
    if ( ky.isUdf() )
	workctio_.setObj( nullptr );
    else
	workctio_.setObj( IOM().get(ky) );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    uiIOObjSelDlg::Setup sdsu( setup_.seltxt_ );
    sdsu.multisel( false )
	.withwriteopts( setup_.withwriteopts_ )
	.withinserters( setup_.withinserters_ )
	.trsnotallwed( setup_.trsnotallwed_ );
    auto* ret = new uiIOObjSelDlg( this, sdsu, workctio_ );
    uiIOObjSelGrp* selgrp = ret->selGrp();
    if ( selgrp )
    {
	selgrp->setConfirmOverwrite( false );
	if ( wrtrselfld_ )
	    selgrp->setDefTranslator( wrtrselfld_->selectedTranslator() );
    }

    if ( !helpkey_.isEmpty() )
	ret->setHelpKey( helpkey_ );

    return ret;
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    if ( !nm || !*nm )
	return nullptr;

    if ( wrtrselfld_ )
	return wrtrselfld_->mkEntry( nm );

    workctio_.setName( nm );
    workctio_.fillObj( false );
    return workctio_.ioobj_ ? workctio_.ioobj_->clone() : nullptr;
}
