/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.19 2004-10-06 14:00:23 bert Exp $
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
    setup.pol2d( SeisSelSetup::Only2D ).selattr( with_attr );
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
					   BufferString& setname )
{
    mGet2DLineSet()
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    mGet2DLineSet(false)
    BufferStringSet linenames;
    for ( int idx=0; idx<lineset.nrLines(); idx++ )
    {
	const char* linenm = lineset.lineName(idx);
	if ( linenames.indexOf(linenm) < 0 )
	    linenames.add( linenm );
    }

    linenames.sort();
    uiListBoxDlg dlg( appserv().parent(), linenames, "Lines" );
    dlg.box()->setMultiSelect();
    if ( !dlg.go() ) return false;
    for ( int idx=0; idx<dlg.box()->size(); idx++ )
    {
	if ( dlg.box()->isSelected(idx) )
	    res.add( dlg.box()->textOfItem(idx) );
    }

    return res.size();
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs )
{
    mGet2DLineSet()
    for ( int idx=0; idx<lineset.nrLines(); idx++ )
    {
	if ( !strcmp(linenm,lineset.lineName(idx)) )
	    attribs.add( lineset.attribute(idx) );
    }
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linenm,
				       const char* attribnm, 
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)
    int lineidx = -1;
    BufferString linekey = Seis2DLineSet::lineKey(linenm,attribnm);
    for ( int idx=0; idx<lineset.nrLines(); idx++ )
    {
	if ( linekey != lineset.lineKey(idx) ) continue;
	lineidx = idx;
	break;
    }

    if ( lineidx < 0 ) return false;
    lineset.getCubeSampling( cs, lineidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lineidx, buf );
    uiExecutor dlg( appserv().parent(), *exec );
    return dlg.go();
}
