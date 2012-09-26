/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseisioobjinfo.h"

#include "cbvsreadmgr.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "systeminfo.h"
#include "uimsg.h"

# include <sstream>


uiSeisIOObjInfo::uiSeisIOObjInfo( const IOObj& ioobj, bool errs )
    	: sii(ioobj)
    	, doerrs(errs)
{
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const MultiID& key, bool errs )
    	: sii(key)
    	, doerrs(errs)
{
}


#define mChk(ret) if ( !isOK() ) return ret

bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChk(false);
    if ( is2D() )
	return true;

    PtrMan<Translator> t = ioObj()->getTranslator();
    if ( !t )
	{ pErrMsg("No Translator"); return true; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,tr,t.ptr());
    if ( !tr )
	{ return true; }

    Conn* conn = ioObj()->getConn( Conn::Read );
    if ( !conn || !tr->initRead(conn) )
    {
	if ( doerrs )
	    uiMSG().error( "No output cube produced" );
	return false;
    }

    std::ostringstream strm;
    tr->readMgr()->dumpInfo( strm, false );
    uiMSG().message( strm.str().c_str() );

    return true;
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SeisIOObjInfo::SpaceInfo& si ) const
{
    mChk(false);

    const int szmb = expectedMBs( si );
    if ( szmb < 0 ) // Unknown, but probably small
	return true;

    const int avszmb = System::getFreeMBOnDisk( *ioObj() );
#ifdef __win__
    const int szgb = szmb / 1024;
    if ( szgb >= 4 )
    {
	BufferString fsysname = System::getFileSystemName( ioObj()->dirName() );
	if ( fsysname == "FAT32" )
	{ 
	    uiMSG().error( "Target directory has a FAT32 File System.\n"
			   "Files larger than 4GB are not supported" );
	    return false;
	}
    }
#endif
	
    if ( avszmb == 0 )
    {
	if ( !doerrs ) return false;
	if ( !uiMSG().askContinue( "The output disk seems to be full.\n"
		    		"Do you want to continue?" ) )
	    return false;
    }
    else if ( szmb > avszmb )
    {
	if ( !doerrs ) return false;
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
	msg += "\nDo you want to continue?";
	if ( !uiMSG().askContinue( msg ) )
	    return false;
    }
    return true;
}
