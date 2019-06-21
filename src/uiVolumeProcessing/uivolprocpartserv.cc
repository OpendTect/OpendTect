/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/

#include "uivolprocpartserv.h"

#include "ioobj.h"
#include "ptrman.h"
#include "separstr.h"
#include "volprocchain.h"
#include "volproctrans.h"

#include "uimsg.h"
#include "uivolprocbatchsetup.h"
#include "uivolprocchain.h"


uiVolProcPartServer::uiVolProcPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , volprocchain_(0)
    , volprocchain2d_(0)
    , volprocchaindlg_(0)
    , volprocchaindlg2d_(0)
    , volprocdlg_(0)
    , volprocdlg2d_(0)
{
}


uiVolProcPartServer::~uiVolProcPartServer()
{
    if ( volprocchain_ ) volprocchain_->unRef();
    if ( volprocchain2d_ ) volprocchain2d_->unRef();
    delete volprocchaindlg_; delete volprocchaindlg2d_;
    delete volprocdlg_; delete volprocdlg2d_;
}


void uiVolProcPartServer::doVolProc( bool is2d, const DBKey* mid,
				     const char* steptype )
{
    VolProc::Chain*& vprocchain = is2d ? volprocchain2d_ : volprocchain_;
    VolProc::uiChain*& vprocdlg = is2d ? volprocchaindlg2d_ : volprocchaindlg_;

    if ( !vprocchain )
    {
	vprocchain = new VolProc::Chain;
	vprocchain->ref();
    }

    PtrMan<IOObj> ioobj = mid ? mid->getIOObj() : 0;
    if ( ioobj )
    {
	uiString errmsg;
	if ( !VolProcessingTranslator::retrieve(*vprocchain,ioobj,errmsg) )
	{
	    uiString fullmsg( ioobj->phrCannotReadObj() );
	    if ( !errmsg.isEmpty() )
		fullmsg.appendPhrase( errmsg );

	    uimsg().error( fullmsg );
	    vprocchain->unRef();
	    vprocchain = new VolProc::Chain;
	    vprocchain->ref();
	}
    }

    if ( !vprocdlg )
    {
	vprocdlg = new VolProc::uiChain( parent(), *vprocchain, is2d, true );
	vprocdlg->windowClosed.notify(
			mCB(this,uiVolProcPartServer,volprocchainDlgClosed) );
    }
    else
	vprocdlg->setChain( *vprocchain );

    vprocdlg->show();
    vprocdlg->raise();
    if ( steptype )
	vprocdlg->addStep( steptype );
}


void uiVolProcPartServer::volprocchainDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(VolProc::uiChain*,dlg,cb)
    const bool is2d = dlg == volprocchaindlg2d_;
    VolProc::Chain* vprocchain = is2d ? volprocchain2d_ : volprocchain_;
    VolProc::uiChain* vprocdlg = is2d ? volprocchaindlg2d_ : volprocchaindlg_;
    if ( !vprocdlg || !vprocdlg->saveButtonChecked() ||
	 !vprocchain || vprocdlg->uiResult()==0 )
	return;

    PtrMan<IOObj> ioobj = vprocchain->storageID().getIOObj();
    createVolProcOutput( is2d, ioobj );
}


void uiVolProcPartServer::createVolProcOutput( bool is2d, const IOObj* sel )
{
    VolProc::uiBatchSetup*& dlg = is2d ? volprocdlg2d_ : volprocdlg_;
    if ( !dlg )
    {
	dlg = new VolProc::uiBatchSetup( parent(), is2d, sel );
	dlg->setModal( false );
    }
    else
	dlg->setIOObj( sel );

    dlg->show();
    dlg->raise();
}


void uiVolProcPartServer::fillPar( IOPar& ) const
{}

bool uiVolProcPartServer::usePar( const IOPar& )
{ return true; }
