/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.14 2004-09-08 07:45:46 bert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"
#include "uimergeseis.h"
#include "uiseissegyimpexp.h"
#include "uiseiscbvsimp.h"
#include "uiseisfileman.h"
#include "uisegysip.h"
#include "uiseissel.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "ptrman.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    	: uiApplPartServer(a)
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider( segyid );
    uiSurveyInfoEditor::addInfoProvider( sip );
}


bool uiSeisPartServer::ioSeis( uiSeisPartServer::ExternalType t, bool forread )
{
    PtrMan<uiDialog> dlg = t == uiSeisPartServer::SegY
      ?	(uiDialog*)new uiSeisSegYImpExp( appserv().parent(), forread, segyid )
      : (uiDialog*)new uiSeisImpCBVS( appserv().parent() );

    return dlg->go();
}


bool uiSeisPartServer::importSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, true ); }
bool uiSeisPartServer::exportSeis()
{ return ioSeis( uiSeisPartServer::SegY, false ); }


bool uiSeisPartServer::mergeSeis()
{
    uiMergeSeis dlg( appserv().parent() );
    return dlg.go();
}


void uiSeisPartServer::manageSeismics()
{
    uiSeisFileMan dlg( appserv().parent() );
    dlg.go();
}


bool uiSeisPartServer::select2DSeis( MultiID& mid )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    SeisSelSetup setup;
    setup.pol2d( SeisSelSetup::Only2D );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DLineGroup(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineGroup grp( fnm );

void uiSeisPartServer::get2DLineInfo( const MultiID& mid, BufferString& setname,
				      BufferStringSet& linenames )
{
    mGet2DLineGroup()
    setname = grp.name();
    for ( int idx=0; idx<grp.nrLines(); idx++ )
    {
	const char* linenm = grp.lineName(idx);
	if ( linenames.indexOf(linenm) < 0 )
	    linenames.add( linenm );
    }
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs )
{
    mGet2DLineGroup()
    for ( int idx=0; idx<grp.nrLines(); idx++ )
    {
	if ( !strcmp(linenm,grp.lineName(idx)) )
	    attribs.add( grp.attribute(idx) );
    }
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linenm,
				       const char* attribnm, SeisTrcBuf& buf )
{
    mGet2DLineGroup(false)
    int lineidx = -1;
    BufferString linekey = Seis2DLineGroup::lineKey(linenm,attribnm);
    for ( int idx=0; idx<grp.nrLines(); idx++ )
    {
	if ( linekey != grp.lineKey(idx) ) continue;
	lineidx = idx;
	break;
    }

    if ( lineidx < 0 ) return false;
    PtrMan<Executor> exec = grp.lineFetcher( lineidx, buf );
    uiExecutor dlg( appserv().parent(), *exec );
    return dlg.go();
}
