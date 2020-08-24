/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uiioobjinserter.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "dbdir.h"
#include "dbman.h"
#include "iopar.h"
#include "staticstring.h"
#include "transl.h"

#include "uimsg.h"
#include "uistatusbar.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uitoolbutton.h"
#include "od_helpids.h"
#include "settings.h"
#include "file.h"
#include "keystrs.h"


mImplClassFactory( uiIOObjInserter, factory );

bool uiIOObjInserter::isPresent( const TranslatorGroup& grp )
{
    const ObjectSet<const Translator>& tpls = grp.templates();

    for ( int idx=0; idx<tpls.size(); idx++ )
	if ( isPresent(*tpls[idx]) )
	    return true;

    return false;
}


static int getDisabledInserters( BufferStringSet& sel )
{
    Settings::common().get( "Ui.Inserters.Enable", sel );
    if ( sel.isEmpty() || sel.get(0) == sKey::All() )
	return 1;
    return sel.get(0) == sKey::None() ? -1 : 0;
}


bool uiIOObjInserter::allDisabled()
{
    BufferStringSet dum;
    return getDisabledInserters(dum) == -1;
}


bool uiIOObjInserter::isDisabled() const
{
    BufferStringSet enabled;
    const int res = getDisabledInserters( enabled );
    if ( res != 0 )
	return res < 0;

    return !enabled.isPresent( name() );
}


void uiIOObjInserter::addInsertersToDlg( uiParent* p,
					 CtxtIOObj& ctio,
					 ObjectSet<uiIOObjInserter>& insertset,
					 ObjectSet<uiButton>& buttonset )
{
    if ( uiIOObjInserter::allDisabled() )
	return;

    const ObjectSet<const Translator>& tpls
			= ctio.ctxt_.trgroup_->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
    {
	uiIOObjInserter* inserter = uiIOObjInserter::create( *tpls[idx] );
	if ( !inserter || inserter->isDisabled() )
	    continue;

	uiToolButtonSetup* tbsu = inserter->getButtonSetup();
	if ( !tbsu )
	    { delete inserter; continue; }

	uiButton* but = tbsu->getButton( p, true );
	if ( but )
	    buttonset += but;

	delete tbsu;
	insertset += inserter;
    }
}



#define mConstructorInitListStart(c) \
	uiIOObjRetDlg(p, uiDialog::Setup(selTxt(c.forread_), \
		    mNoDlgTitle, mODHelpKey(mIOObjSelDlgHelpID) ) \
	    .nrstatusflds(1)) \
    , selgrp_( 0 ) \
    , crctio_( 0 )


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const IOObjContext& ctxt )
    : mConstructorInitListStart(ctxt)
    , setup_(uiString::empty())
{
    crctio_ = new CtxtIOObj( ctxt );
    init( *crctio_ );
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& ctio,
				const uiString& ttxt )
    : mConstructorInitListStart(ctio.ctxt_)
    , setup_( ttxt )
{
    init( ctio );
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const uiIOObjSelDlg::Setup& su,
				const CtxtIOObj& ctio )
    : mConstructorInitListStart(ctio.ctxt_)
    , setup_( su )
{
    init( ctio );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    if ( crctio_ )
	{ delete crctio_->ioobj_; delete crctio_; }
}


uiString uiIOObjSelDlg::selTxt( bool forread )
{
    return forread
       ? uiStrings::sInputSelection()
       : uiStrings::sOutputSelection();
}


void uiIOObjSelDlg::init( const CtxtIOObj& ctio )
{
    uiIOObjSelGrp::Setup sgsu( ctio.ctxt_.forread_ && setup_.multisel_
			? OD::ChooseAtLeastOne : OD::ChooseOnlyOne );
    sgsu.allowsetdefault( setup_.allowsetsurvdefault_ );
    sgsu.withwriteopts( setup_.withwriteopts_ );
    sgsu.withinserters( setup_.withinserters_ );
    selgrp_ = new uiIOObjSelGrp( this, ctio, sgsu );
    selgrp_->getListField()->setHSzPol( uiObject::MedVar );
    statusBar()->setTxtAlign( 0, OD::Alignment::Right );
    selgrp_->newStatusMsg.notify( mCB(this,uiIOObjSelDlg,statusMsgCB));

    uiString titletext( setup_.titletext_ );
    if ( titletext.isEmpty() )
    {
	if ( selgrp_->getContext().forread_ )
	    titletext = uiStrings::phrSelect(uiStrings::sInput().toLower());
	else
	    titletext = uiStrings::phrSelect(uiStrings::sOutput().toLower());

	if ( selgrp_->getContext().name().isEmpty() )
	    titletext.postFixWord( toUiString(ctio.ctxt_.trgroup_->typeName()));
	else
	    titletext.postFixWord( toUiString(ctio.ctxt_.name()) );

	if ( setup_.multisel_ )
	    titletext.withUnit( uiStrings::sTimeUnitString() );
    }
    setTitleText( titletext );

    uiString captn;
    if ( !selgrp_->getContext().forread_ )
	captn = tr("Save %1 as" );
    else
    {
	if ( setup_.multisel_ )
	    captn = tr("Load one or more %1");
	else
	    captn = tr("Load %1");
    }

    if ( selgrp_->getContext().name().isEmpty() )
	captn = captn.arg( ctio.ctxt_.trgroup_->typeName() );
    else
	captn = captn.arg( ctio.ctxt_.name() );
    setCaption( captn );

    selgrp_->getListField()->doubleClicked.notify(
	    mCB(this,uiDialog,accept) );
}


const IOObj* uiIOObjSelDlg::ioObj() const
{
    selgrp_->updateCtxtIOObj();
    return selgrp_->getCtxtIOObj().ioobj_;
}


void uiIOObjSelDlg::statusMsgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( toUiString(msg) );
}


void uiIOObjSelDlg::setConfirmOverwrite( bool yn )
{
    selgrp_->setConfirmOverwrite( yn );
}


void uiIOObjSelDlg::setSurveyDefaultSubsel( const char* subsel )
{
    selgrp_->setSurveyDefaultSubsel( subsel );
}


#define mSelTxt(txt,ct) \
    !txt.isEmpty() ? txt \
	: toUiString(ct.name().isEmpty() \
	    ? ct.translatorGroupName().buf() : ct.name().buf())

uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c, const uiString& txt)
    : uiIOSelect(p,uiIOSelect::Setup(mSelTxt(txt,c)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(mSelTxt(txt,c))
    , inctiomine_(true)
{ init(); }


uiIOObjSel::uiIOObjSel( uiParent* p, const IOObjContext& c,
			const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(*new CtxtIOObj(c))
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(true)
{ init(); }


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiString& txt )
    : uiIOSelect(p,uiIOSelect::Setup(mSelTxt(txt,c.ctxt_)),
		 mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(mSelTxt(txt,c.ctxt_))
    , inctiomine_(false)
{ init(); }


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(false)
{ init(); }


void uiIOObjSel::init()
{
    const IOObjContext& ctxt = workctio_.ctxt_;
    inp_->setEditable( !ctxt.forread_ );

    ctxt.fillTrGroup();
    wrtrselfld_ = 0; usesharedbut_ = 0;
    if ( ctxt.forread_ && setup_.withinserters_ )
    {
	uiIOObjInserter::addInsertersToDlg( this, workctio_, inserters_,
					    insbuts_ );
	for ( int idx=0; idx<inserters_.size(); idx++ )
	    inserters_[idx]->objectInserted.notify(
					mCB(this,uiIOObjSel,objInserted) );
    }
    else if ( !ctxt.forread_ )
    {
	uiObject* curendobj = uiIOSelect::endObj( false );
	if ( setup_.withwriteopts_ )
	{
	    wrtrselfld_ = new uiIOObjSelWriteTranslator( this, workctio_,
							 false );
	    wrtrselfld_->attach( rightOf, uiIOSelect::endObj(false) );
	    curendobj = wrtrselfld_->attachObj();
	}
	if ( ctxt.destpolicy_ != IOObjContext::SurveyOnly )
	{
	    usesharedbut_ = new uiCheckBox( this, uiString::empty() );
	    usesharedbut_->setIcon( "shared_data" );
	    usesharedbut_->setChecked(
			    ctxt.destpolicy_ == IOObjContext::PreferShared );
	    usesharedbut_->setToolTip(
	      tr("Put the output data in the common (shared) data directory") );
	    usesharedbut_->attach( rightOf, curendobj );
	}
    }
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
    mAttachCB( DBM().afterSurveyChange, uiIOObjSel::survChangedCB );
    mAttachCB( DBM().entryAdded, uiIOObjSel::dbmChgCB );
    mAttachCB( DBM().entryRemoved, uiIOObjSel::dbmChgCB );
}


uiIOObjSel::~uiIOObjSel()
{
    detachAllNotifiers();
    deepErase( inserters_ );
    if ( inctiomine_ )
	{ delete inctio_.ioobj_; delete &inctio_; }
    delete workctio_.ioobj_; delete &workctio_;
}


bool uiIOObjSel::forRead() const
{
    return workctio_.ctxt_.forread_;
}


void uiIOObjSel::preFinaliseCB( CallBacker* )
{
    initRead();
}


void uiIOObjSel::survChangedCB( CallBacker* )
{
    if ( workctio_.ioobj_ )
	deleteAndZeroPtr( workctio_.ioobj_ );

    initRead();
}


void uiIOObjSel::dbmChgCB( CallBacker* cb )
{
    mCBCapsuleUnpack( DBKey, id, cb );
    if ( id.isInvalid() )
	return;

    if ( workctio_.ctxt_.getSelDirID() == id.dirID() )
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
    if ( !left )
    {
	if ( usesharedbut_ )
	    return usesharedbut_;
	else if ( wrtrselfld_ && !wrtrselfld_->isEmpty() )
	    return wrtrselfld_->attachObj();
    }
    return uiIOSelect::endObj( left );
}


void uiIOObjSel::fillDefault()
{
    if ( setup_.filldef_ && !workctio_.ioobj_ && forRead() )
	workctio_.fillDefault();
}


void uiIOObjSel::fillEntries()
{
    if ( !inctio_.ctxt_.forread_ )
	return;

    const bool hadselioobj = workctio_.ioobj_;

    DBDirEntryList entrylist( inctio_.ctxt_ );
    BufferStringSet keys, names;
    if ( setup_.withclear_ || !setup_.filldef_ )
	{ keys.add( "" ); names.add( "" ); }

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	keys.add( entrylist.key(idx).toString() );
	names.add( entrylist.name(idx) );
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


bool uiIOObjSel::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::ID(),
	       workctio_.ioobj_ ? workctio_.ioobj_->key() : DBKey() );
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
	const DBKey dbky( res );
	workctio_.setObj( dbky );
	setInput( dbky );
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


void uiIOObjSel::setInput( const DBKey& dbky )
{
    workctio_.setObj( dbky.getIOObj() );
    uiIOSelect::setInput( dbky.toString() );
    if ( workctio_.ioobj_ && wrtrselfld_ )
        wrtrselfld_->use(*workctio_.ioobj_);
}


void uiIOObjSel::setInput( const IOObj& ioobj )
{
    setInput( ioobj.key() );
}


void uiIOObjSel::updateInput()
{
    setInput( workctio_.ioobj_ ? workctio_.ioobj_->key() : DBKey::getInvalid());
}


void uiIOObjSel::setEmpty()
{
    uiIOSelect::setEmpty();
    workctio_.setObj( 0 ); inctio_.setObj( 0 );
    updateInput();
}


const char* uiIOObjSel::userNameFromKey( const char* kystr ) const
{
    mDeclStaticString( nm );
    nm.setEmpty();
    const DBKey dbky( kystr );
    if ( dbky.isValid() )
	nm = dbky.name();
    return nm.buf();
}


void uiIOObjSel::obtainIOObj()
{
    StringPair strpair( getInput() );
    const BufferString inp( strpair.first() );
    if ( inp.isEmpty() )
	{ workctio_.setObj( 0 ); return; }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( ( inp == itemusrnm || strpair.getCompString() == itemusrnm )
		&& workctio_.ioobj_ && workctio_.ioobj_->name()==inp.buf() )
	    return;
    }

    PtrMan<IOObj> ioob = DBM().getByName( workctio_.ctxt_, inp );
    workctio_.setObj( ioob && workctio_.ctxt_.validIOObj(*ioob)
		    ? ioob.release() : 0 );
}


void uiIOObjSel::processInput()
{
    obtainIOObj();
    if ( workctio_.ioobj_ || forRead() )
	updateInput();
}


bool uiIOObjSel::existingUsrName( const char* nm ) const
{
    PtrMan<IOObj> obj = DBM().getByName( workctio_.ctxt_, nm );
    return obj;
}


DBKey uiIOObjSel::getKeyOnly() const
{
    PtrMan<IOObj> ioob = DBM().getByName( workctio_.ctxt_, getInput() );
    return ioob ? ioob->key() : DBKey();
}


void uiIOObjSel::doCommit( bool noerr ) const
{
    bool alreadyerr = noerr;
    const_cast<uiIOObjSel*>(this)->doCommitInput(alreadyerr);
    if ( !setup_.optional_ && !inctio_.ioobj_ && !alreadyerr )
    {
	CtxtIOObj ctio( inctio_.ctxt_ );
	if ( !ctio.ctxt_.forread_ && !ctio.ioobj_ )
	{
	    uiMSG().error( uiStrings::phrCannotCreateDBEntryFor(
			    workctio_.ctxt_.uiObjectTypeName()) );
	    return;
	}
	uiString txt(inctio_.ctxt_.forread_
			 ? uiStrings::phrSelect(toUiString("%1"))
			 : uiStrings::phrEnter(tr("a valid name for the %1" )));
	uiMSG().error( txt.arg( workctio_.ctxt_.uiObjectTypeName() ) );
    }
}


DBKey uiIOObjSel::key( bool noerr ) const
{
    doCommit(noerr);
    return inctio_.ioobj_ ? inctio_.ioobj_->key() : DBKey::getInvalid();
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
    StringPair strpair( getInput() );
    const BufferString inp( strpair.first() );
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
	if ( workctio_.ioobj_ )
	{
	    if ( !forRead() && wrtrselfld_
		&& !wrtrselfld_->isEmpty()
		&& !wrtrselfld_->hasSelectedTranslator(*workctio_.ioobj_) )
		mErrRet( tr("Cannot change the output format "
			 "for an already existing entry") )

	    const bool isalreadyok = inctio_.ioobj_
			    && inctio_.ioobj_->key() == workctio_.ioobj_->key();
	    if ( !alreadyerr && !isalreadyok && !forRead() )
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
			mErrRet(uiString::empty())
		}
	    }

	    inctio_.setObj( workctio_.ioobj_->clone() );
	    commitSucceeded(); return true;
	}

	mErrRet(tr("'%1' already exists as another object type."
	       "\nPlease enter another name.").arg(getInput()))

    }
    if ( forRead() )
	return false;

    workctio_.setObj( createEntry( getInput() ) );
    inctio_.setObj( workctio_.ioobj_ ? workctio_.ioobj_->clone() : 0 );
    if ( !inctio_.ioobj_ ) return false;

    commitSucceeded();
    return true;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    if ( !forRead() )
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

    delete dlg;
}


void uiIOObjSel::objInserted( CallBacker* cb )
{
    mCBCapsuleUnpack( DBKey, ky, cb );
    if ( ky.isValid() )
	setInput( ky );
}


void uiIOObjSel::objSel()
{
    const char* kystr = getKey();
    if ( !kystr || !*kystr )
	workctio_.setObj( 0 );
    else
	workctio_.setObj( DBKey(kystr).getIOObj() );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    uiIOObjSelDlg::Setup sdsu( setup_.seltxt_ );
    sdsu.multisel( false )
	.withwriteopts( setup_.withwriteopts_ )
	.withinserters( setup_.withinserters_ );
    uiIOObjSelDlg* ret = new uiIOObjSelDlg( this, sdsu, workctio_ );
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
	return 0;

    const bool mkinshared = usesharedbut_ && usesharedbut_->isChecked();
    if ( wrtrselfld_ )
	return wrtrselfld_->mkEntry( nm, mkinshared );

    workctio_.ctxt_.destpolicy_ = mkinshared
	    ? IOObjContext::PreferShared : IOObjContext::AllowShared;
    workctio_.setName( nm );
    workctio_.fillObj( false );
    return workctio_.ioobj_ ? workctio_.ioobj_->clone() : 0;
}
