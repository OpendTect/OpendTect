/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uiioobjselwritetransl.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "linekey.h"
#include "transl.h"

#include "uimsg.h"
#include "uistatusbar.h"
#include "uilistbox.h"
#include "od_helpids.h"


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const uiString& seltxt, bool multisel,
			      bool havesetsurvdefault )
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread ? tr("Input selection")
				     : tr("Output selection"),
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

	titletext = titletext.arg( multisel ? "(s)"
					    : uiStrings::sEmptyString() );
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
    crWriteTranslSelFld();
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
    crWriteTranslSelFld();
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
    crWriteTranslSelFld();
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const uiIOObjSel::Setup& su )
    : uiIOSelect(p,su,mCB(this,uiIOObjSel,doObjSel))
    , inctio_(c)
    , workctio_(*new CtxtIOObj(c))
    , setup_(su)
    , inctiomine_(false)
{
    crWriteTranslSelFld();
    preFinalise().notify( mCB(this,uiIOObjSel,preFinaliseCB) );
}


void uiIOObjSel::crWriteTranslSelFld()
{
    workctio_.ctxt.fillTrGroup();
    if ( workctio_.ctxt.forread )
	wrtrselfld_ = 0;
    else
    {
	wrtrselfld_ = new uiIOObjSelWriteTranslator( this, workctio_, false );
	wrtrselfld_->attach( rightOf, uiIOSelect::endObj(false) );
    }
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


uiObject* uiIOObjSel::endObj( bool left )
{
    if ( wrtrselfld_ && !left && !wrtrselfld_->isEmpty() )
	return wrtrselfld_->attachObj();
    return uiIOSelect::endObj( left );
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
	uiMSG().error( txt.arg( workctio_.ctxt.objectTypeName() ) );
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
	    if ( wrtrselfld_ && !wrtrselfld_->hasSelectedTranslator(
							*workctio_.ioobj ) )
		mErrRet( "Cannot change the output format "
			 "for an already existing entry" )

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
    if ( workctio_.ctxt.forread )
	return false;

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
	if ( wrtrselfld_ )
	    wrtrselfld_->use( *workctio_.ioobj );
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
    return !wrtrselfld_ ? 0 : wrtrselfld_->mkEntry( nm );
}
