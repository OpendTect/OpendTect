/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocpartserv.h"

#include "ioman.h"
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
    , volprocchaindlg_(0)
{
}


uiVolProcPartServer::~uiVolProcPartServer()
{
    if ( volprocchain_ ) volprocchain_->unRef();
    delete volprocchaindlg_;
}


void uiVolProcPartServer::doVolProc( const MultiID* mid )
{
    if ( !volprocchain_ )
    {
	volprocchain_ = new VolProc::Chain;
	volprocchain_->ref();
    }

    PtrMan<IOObj> ioobj = mid ? IOM().get( *mid ) : 0;
    if ( ioobj )
    {
	BufferString errmsg;
	if ( !VolProcessingTranslator::retrieve( *volprocchain_, ioobj,
		errmsg ) )
	{
	    FileMultiString fms( "Cannot read volume builder setup" );
	    if ( errmsg.buf() )
		fms += errmsg;

	    uiMSG().error( fms.buf() );
	    volprocchain_->unRef();
	    volprocchain_ = new VolProc::Chain;
	    volprocchain_->ref();
	}
    }

    if ( !volprocchaindlg_ )
    {
	volprocchaindlg_ = new VolProc::uiChain( parent(), *volprocchain_,true);
	volprocchaindlg_->windowClosed.notify(
		mCB(this,uiVolProcPartServer,volprocchainDlgClosed) );
    }
    else
	volprocchaindlg_->setChain( *volprocchain_ );

    volprocchaindlg_->raise();
    volprocchaindlg_->show();
}


void uiVolProcPartServer::volprocchainDlgClosed( CallBacker* )
{
    if ( !volprocchaindlg_ || !volprocchaindlg_->saveButtonChecked() ||
	 !volprocchain_ || volprocchaindlg_->uiResult()==0 )
	return;

    PtrMan<IOObj> ioobj = IOM().get( volprocchain_->storageID() );
    createVolProcOutput( ioobj );
}


void uiVolProcPartServer::createVolProcOutput( const IOObj* sel )
{
    VolProc::uiBatchSetup dlg( parent(), sel );
    dlg.go();
}


void uiVolProcPartServer::fillPar( IOPar& ) const
{}

bool uiVolProcPartServer::usePar( const IOPar& )
{ return true; }
