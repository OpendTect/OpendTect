/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.30 2005-02-28 14:49:40 cvsbert Exp $
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
#include "iodir.h"
#include "ioman.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "ptrman.h"
#include "uilistboxdlg.h"
#include "uimenu.h"
#include "seispsioprov.h"
#include "seisbuf.h"
#include "seispsread.h"
#include "seissectview.h"



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
    dlg.box()->selectAll( true );
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
    if ( lineidx < 0 )
    {
	BufferStringSet attribs;
	get2DStoredAttribs( mid, linenm, attribs );
	if ( !attribs.size() ) return false;
	lineidx = lineset.indexOf( LineKey(linenm,attribs.get(0)) );
	if ( lineidx < 0 ) return false;
    }

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


BufferStringSet uiSeisPartServer::getStoredGathersList()
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    BufferStringSet ioobjnms;

    const char* psstring = "Pre-Stack Seismics";
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( strcmp(ioobj.group(),psstring) ) continue;
	ioobjnms.add( (const char*)ioobj.name() );
	if ( ioobjnms.size() > 1 )
	{
	    for ( int icmp=ioobjnms.size()-2; icmp>=0; icmp-- )
	    {
		if ( ioobjnms.get(icmp) > ioobjnms.get(icmp+1) )
		{
		    BufferString* tmp = ioobjnms[icmp];
		    ioobjnms.replace( ioobjnms[icmp+1], icmp );
		    ioobjnms.replace( tmp, icmp+1  );
		}
	    }
	}
    }
    
    return ioobjnms;
}

#define mInsertItems(list,mnu,correcttype) \
mnu->setEnabled( list.size() ); \
    for ( int idx=start; idx<stop; idx++ ) \
{ \
    const BufferString& nm = list.get(idx); \
    uiMenuItem* itm = new uiMenuItem( nm ); \
    mnu->insertItem( itm, mnuid, idx ); \
    mnuid++; \
}


uiPopupMenu* uiSeisPartServer::createStoredGathersSubMenu( int& mnuid )
{
    BufferStringSet ioobjnms = getStoredGathersList();
    if ( !ioobjnms.size() ) return 0;
    uiPopupMenu* displaygathermnu = new uiPopupMenu( appserv().parent(),
		                                     "Display Gather" );
    const int start = 0; const int stop = ioobjnms.size();
    mInsertItems(ioobjnms,displaygathermnu,true);
    return displaygathermnu;
}
	    

bool uiSeisPartServer::handleGatherSubMenu( int mnuid, BinID bid )
{
    BufferStringSet ioobjnms = getStoredGathersList();
    PtrMan<IOObj> ioobj = IOM().getLocal( ioobjnms.get(mnuid) );
    if ( !ioobj )
    { 
	uiMSG().error( "No valid gather selected" ); 
	return false;
    }

    SeisPSReader* rdr = SPSIOPF().getReader( *ioobj, bid.inl );
    if ( !rdr )
    {
	uiMSG().error( "This Pre-Stack data store cannot be handeled" );
        return false;
    }
    
    SeisTrcBuf tbuf;
    if ( !rdr->getGather(bid,tbuf) )
    {
        uiMSG().error( rdr->errMsg() );
        return false;
    }
    
    SeisSectionViewer svw;
    svw.title = "Gather from ["; svw.title += ioobj->name();
    svw.title += "] at ";
    svw.title += bid.inl; svw.title += "/"; svw.title += bid.crl;
    svw.minimal = true;
    
    PtrMan<Executor> exec = svw.preparer();
    if ( !exec->execute() )
    {
        uiMSG().error( exec->message() );
        return false;
    }

    for ( int idx=0; idx<tbuf.size(); idx++ )
        svw.add( tbuf.get(idx) );

    svw.close();
    return true;
}
