/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2004
 RCS:		$Id: uiseisioobjinfo.cc,v 1.6 2004-08-27 10:07:33 bert Exp $
________________________________________________________________________

-*/

#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "seistrcsel.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "conn.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "cubesampling.h"
#include "errh.h"

# include <sstream>

uiSeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange().nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = SI().sampling(false).hrg.totalNr();
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const IOObj& ioobj, bool errs )
    	: ctio(*mMkCtxtIOObj(SeisTrc))
    	, doerrs(errs)
{
    ctio.ioobj = ioobj.clone();
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const MultiID& key, bool errs )
    	: ctio(*mMkCtxtIOObj(SeisTrc))
    	, doerrs(errs)
{
    ctio.ioobj = IOM().get( key );
}


uiSeisIOObjInfo::~uiSeisIOObjInfo()
{
    delete ctio.ioobj;
    delete &ctio;
}


#define mChkIOObj(ret) \
    if ( doerrs && !ctio.ioobj ) \
    { \
	uiMSG().error( "Cannot find object in data store" ); \
	return ret; \
    }


bool uiSeisIOObjInfo::is2D() const
{
    mChkIOObj(false);
    return SeisTrcTranslator::is2D( *ctio.ioobj );
}


bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChkIOObj(false);

    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    if ( !t )
	{ pFreeFnErrMsg("No Translator","provideUserInfo"); return true; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,tr,t.ptr());
    if ( !tr )
	{ pFreeFnErrMsg("Non-CBVS entry","provideUserInfo"); return true; }

    Conn* conn = ctio.ioobj->getConn( Conn::Read );
    if ( !conn || !tr->initRead(conn) )
    {
	if ( doerrs )
	    uiMSG().error( "Cannot open seismic data file(s)" );
	delete conn;
	return false;
    }

    std::ostringstream strm;
    tr->readMgr()->dumpInfo( strm, false );
    uiMSG().message( strm.str().c_str() );

    return true;
}


int uiSeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChkIOObj(-1);

    Translator* tr = ctio.ioobj->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
    {
	pFreeFnErrMsg("No Translator!","uiSeisIOObjInfo::expectedMBs");
	return -1;
    }

    int overhead = sttr->bytesOverheadPerTrace();
    delete tr;
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    static const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SpaceInfo& si ) const
{
    mChkIOObj(false);

    const int szmb = expectedMBs( si );
    const int avszmb = GetFreeMBOnDisk( ctio.ioobj );
    if ( szmb > avszmb )
    {
	BufferString msg( "The new cube size may exceed the space "
			   "available on disk:\n" );
	if ( avszmb == 0 )
	    msg = "The disk seems to be full!";
	else
	{
	    msg += "\nEstimated size: "; msg += szmb;
	    msg += " MB\nAvailable on disk: "; msg += avszmb;
	    msg += " MB";
	}
	msg += "\nDo you wish to continue?";
	if ( !uiMSG().askGoOn( msg ) )
	    return false;
    }
    return true;
}


bool uiSeisIOObjInfo::getRanges( CubeSampling& cs ) const
{
    mChkIOObj(false);
    if ( SeisTrcTranslator::getRanges( *ctio.ioobj, cs ) )
	return true;

    if ( doerrs )
    {
	BufferString msg( "Cannot read \"" );
	msg += ctio.ioobj->name(); msg += "\"";
	uiMSG().error( msg );
    }
    return false;
}


bool uiSeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChkIOObj(false);

    Translator* tr = ctio.ioobj->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
    {
	pFreeFnErrMsg("No Translator!","uiSeisIOObjInfo::getBPS");
	return false;
    }

    Conn* conn = ctio.ioobj->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
	    	= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}
