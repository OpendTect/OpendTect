/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.14 2003-11-07 12:22:02 bert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uibinidsubsel.h"
#include "uigeninput.h"
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


uiSeisTransfer::uiSeisTransfer( uiParent* p, bool with_format, bool wstp )
	: uiGroup(p,"Seis transfer pars")
{
    subselfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
	    				.withz(true).withstep(wstp) );
    scfmtfld = new uiSeisFmtScale( this, with_format );
    scfmtfld->attach( alignedBelow, subselfld );

    remnullfld = new uiGenInput( this, "Null traces",
	    			BoolInpSpec("Discard","Pass") );
    remnullfld->attach( alignedBelow, scfmtfld );

    setHAlignObj( remnullfld );
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    BinIDSampler bs; StepInterval<float> zrg;
    bs.copyFrom( SI().range() );
    bs.step = BinID( SI().inlStep(), SI().crlStep() );
    assign( zrg, SI().zRange() );
    if ( SeisTrcTranslator::getRanges( ioobj, bs, zrg ) )
	subselfld->setInput( bs, zrg );

    scfmtfld->updateFrom( ioobj );
}


void uiSeisTransfer::setSteering( bool yn )
{
    scfmtfld->setSteering(yn);
}


int uiSeisTransfer::expectedNrTraces() const
{
    return subselfld->expectedNrTraces();
}


int uiSeisTransfer::expectedNrSamples() const
{
    return subselfld->expectedNrSamples();
}


int uiSeisTransfer::maxBytesPerSample() const
{
    DataCharacteristics dc(
	    (DataCharacteristics::UserType)scfmtfld->getFormat() );
    return (int)dc.nrBytes();
}


Executor* uiSeisTransfer::getTrcProc( const IOObj* inobj, const IOObj* outobj,
				      const char* extxt,
				      const char* worktxt ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(outobj) );
    IOPar iop; 
    if ( !subselfld->fillPar(iop) )
	return 0;

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( inobj, outobj, extxt,
	    						&iop, worktxt );
    PtrMan<BinIDRange> brg = subselfld->getRange();
    PtrMan<BinIDProvider> prov = brg->provider();
    stp->setTotalNrIfUnknown( prov->size() );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( remnullfld->getBoolValue() );

    return stp;
}


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

    ostringstream strm;
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
