/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emsurfaceio.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "plugins.h"
#include "position.h"
#include "ranges.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seiszaxisstretcher.h"


static EM::Horizon* load3DHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    EM::dgbSurfaceReader* reader =
	new EM::dgbSurfaceReader( *ioobj, EM::Horizon3D::typeStr() );
    if ( !reader->isOK() )
    {
	delete reader;
	return 0;
    }

    EM::EMObject* emobj = EM::Horizon3D::create( EM::EMM() );
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    emobj->setMultiID( ioobj->key() );
    reader->setOutput( *horizon );
    reader->setReadOnlyZ( true );
    reader->execute( &std::cerr );

    horizon->ref();
    delete reader;
    return horizon;
}


static EM::Horizon* load2DHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    EM::dgbSurfaceReader* reader =
	new EM::dgbSurfaceReader( *ioobj, EM::Horizon2D::typeStr() );
    if ( !reader->isOK() )
    {
	delete reader;
	return 0;
    }

    EM::EMObject* emobj = EM::Horizon2D::create( EM::EMM() );
    mDynamicCastGet(EM::Horizon2D*,horizon,emobj)
    emobj->setMultiID( ioobj->key() );
    reader->setOutput( *horizon );
    reader->setReadOnlyZ( true );
    reader->execute( &std::cerr );

    horizon->ref();
    delete reader;
    return horizon;
}


static IOObj* getIOObjFromPars(  const char* bsky, bool mknew,
				 const IOObjContext& ctxt,
				 bool msgiffail,
				 od_ostream& strm ) const
{
    uiString errmsg;
    IOObj* ioobj = DBM().getFromPar( pars(), bsky, ctxt, mknew, errmsg );
    if ( !ioobj && msgiffail )
    {
	if ( errmsg.isEmpty() )
	    errmsg = tr("Error getting info from database");
	strm << toString(errmsg) << od_endl;
    }

    return ioobj;
}


#define mErrRet(msg)	{ std::cerr << msg << std::endl; return false; }

mLoad1Module("EMAttrib")
{
    PtrMan<IOObj> inioobj = getIOObjFromPars( "Input Seismics", false,
		SeisTrcTranslatorGroup::ioContext(), true );
    SeisIOObjInfo info( inioobj );
    const bool is2d = info.is2D();

    IOObjContext ctxt = SeisTrcTranslatorGroup::ioContext();
    if ( is2d ) ctxt.deftransl = "2D";
    PtrMan<IOObj> outioobj = getIOObjFromPars( "Output Seismics", true,
					       ctxt, true, strm );
    if ( !outioobj || !inioobj )
	mErrRet("Need input and output seismics")

    BufferString mid;
    pars().get( "Reference horizon", mid );

    float refz = 0;
    pars().get( "Reference depth", refz );

    BufferString errmsg;
    EM::Horizon* horizon = is2d ? load2DHorizon( mid.buf(), errmsg )
				: load3DHorizon( mid.buf(), errmsg );
    if ( errmsg != "" ) mErrRet( errmsg )

    CubeSampling cs;
    if ( !cs.hrg.usePar( pars() ) )
	mErrRet( "Cannot read output range" );

    RefMan<EM::HorizonZTransform> zt = new EM::HorizonZTransform( horizon );
    const Interval<float> zrg = zt->getZInterval( false );
    float shift = 0;
    cs.zrg.start = zrg.start + shift;
    cs.zrg.stop = zrg.stop + shift;

    SeisZAxisStretcher exec( *inioobj, *outioobj, cs, *zt, true );
    exec.execute( &strm );
    return true;
}
