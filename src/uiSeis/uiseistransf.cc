/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.11 2003-05-22 11:10:27 bert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uibinidsubsel.h"
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

#include "uigeninput.h"


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


bool uiSeisTransfer::provideUserInfo( const IOObj& ioobj ) const
{
    Translator* t = ioobj.getTranslator();
    if ( !t )
	{ pErrMsg("Cannot get Translator"); return false; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,tr,t);
    if ( !tr )
	{ pErrMsg("Non-CBVS entry"); return true; }

    Conn* conn = ioobj.getConn( Conn::Read );
    if ( !conn || !tr->initRead(conn) )
    {
	uiMSG().error( "Cannot open imported seismic data files" );
	delete conn;
	return false;
    }

    ostringstream strm;
    tr->readMgr()->dumpInfo( strm, false );
    uiMSG().message( strm.str().c_str() );

    return true;
}
