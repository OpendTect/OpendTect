/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.16 2004-06-28 16:00:06 bert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uibinidsubsel.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "conn.h"
#include "errh.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, bool with_format )
	: uiGroup(p,"Seis transfer pars")
	, is2d(false)
	, issteer(false)
{
    scfmtfld = new uiSeisFmtScale( this, with_format );

    remnullfld = new uiGenInput( this, "Null traces",
	    			BoolInpSpec("Discard","Pass") );
    remnullfld->attach( alignedBelow, scfmtfld );

    setHAlignObj( remnullfld );
    mainwin()->finaliseDone.notify( mCB(this,uiSeisTransfer,updFldsForType) );
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    set2D( SeisTrcTranslator::is2D(ioobj) );
    scfmtfld->updateFrom( ioobj );
}


void uiSeisTransfer::updFldsForType( CallBacker* )
{
    scfmtfld->setSteering( issteer );
    scfmtfld->set2D( is2d );
}


void uiSeisTransfer::setSteering( bool yn )
{
    issteer = yn;
    updFldsForType(0);
}


void uiSeisTransfer::set2D( bool yn )
{
    is2d = yn;
    updFldsForType(0);
}


int uiSeisTransfer::maxBytesPerSample() const
{
    if ( issteer ) return 4;
    DataCharacteristics dc(
	    (DataCharacteristics::UserType)scfmtfld->getFormat() );
    return (int)dc.nrBytes();
}


Executor* uiSeisTransfer::getTrcProc( const SeisSelData& insel,
				      const IOObj* outobj,
				      const char* extxt,
				      const char* worktxt ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(outobj) );
    SeisSingleTraceProc* stp = new SeisSingleTraceProc( insel, outobj, extxt,
	    						worktxt );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( !is2d && remnullfld->getBoolValue() );

    return stp;
}


/* Put this where?

# include <sstream>


bool uiSeisTransfer::provideUserInfo( const IOObj& ioobj )
{
    PtrMan<Translator> t = ioobj.getTranslator();
    if ( !t )
	{ pFreeFnErrMsg("No Translator","provideUserInfo"); return true; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,tr,t.ptr());
    if ( !tr )
	{ pFreeFnErrMsg("Non-CBVS entry","provideUserInfo"); return true; }

    Conn* conn = ioobj.getConn( Conn::Read );
    if ( !conn || !tr->initRead(conn) )
    {
	uiMSG().error( "Cannot open created seismic data file(s)" );
	delete conn;
	return false;
    }

    std::ostringstream strm;
    tr->readMgr()->dumpInfo( strm, false );
    uiMSG().message( strm.str().c_str() );

    return true;
}

int uiSeisTransfer::expectedMBs( const IOObj& ioobj ) const
{
    return expectedMBs( ioobj, expectedNrSamples(), maxBytesPerSample(),
		        expectedNrTraces() );
}


bool uiSeisTransfer::checkSpaceLeft( const IOObj& ioobj ) const
{
    return checkSpaceLeft( ioobj, expectedNrSamples(), maxBytesPerSample(),
	    		   expectedNrTraces() );
}


int uiSeisTransfer::expectedMBs( const IOObj& ioobj, int expnrsamps,
				 int maxbps, int expnrtrcs )
{
    Translator* tr = ioobj.getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
    {
	pFreeFnErrMsg("No Translator!","uiSeisTransfer::expectedMBs");
	return -1;
    }

    int overhead = sttr->bytesOverheadPerTrace();
    double sz = expnrsamps;
    sz *= maxbps;
    sz = (sz + overhead) * expnrtrcs;

    static const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


bool uiSeisTransfer::checkSpaceLeft( const IOObj& ioobj, int expnrsamps,
				    int maxbps, int expnrtrcs )
{
    const int szmb = expectedMBs( ioobj, expnrsamps, maxbps, expnrtrcs );
    const int avszmb = GetFreeMBOnDisk( &ioobj );
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

*/
