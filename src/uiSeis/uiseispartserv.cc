/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.25 2004-10-21 15:42:19 nanne Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"
#include "uimergeseis.h"
#include "uiseissegyimpexp.h"
#include "uiseiscbvsimp.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uisegysip.h"
#include "uiseissel.h"
#include "ctxtioobj.h"
#include "seistrcsel.h"
#include "ioobj.h"
#include "ioman.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "ptrman.h"
#include "uilistboxdlg.h"


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


bool uiSeisPartServer::select2DSeis( MultiID& mid, bool with_attr )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    SeisSelSetup setup;
    setup.pol2d( Only2D ).selattr( with_attr );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DLineSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiSeisPartServer::get2DLineSetName( const MultiID& mid, 
					 BufferString& setname ) const
{
    mGet2DLineSet()
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getLineNames( linenames );
    linenames.sort();

    uiListBoxDlg dlg( appserv().parent(), linenames, "Lines" );
    dlg.box()->setMultiSelect();
    if ( !dlg.go() ) return false;
    dlg.box()->getSelectedItems( res );
    return res.size();
}


bool uiSeisPartServer::get2DLineGeometry( const MultiID& mid,
					  const char* linenm,
					  Line2DGeometry& geom ) const
{
    mGet2DLineSet(false)
    int lineidx = lineset.indexOf( linenm );
    if ( lineidx < 0 ) return false;
    return lineset.getGeometry( lineidx, geom );
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs ) const
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)

    int lidx = lineset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    lineset.getCubeSampling( cs, lidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lidx, buf );
    uiExecutor dlg( appserv().parent(), *exec );
    return dlg.go();
}
