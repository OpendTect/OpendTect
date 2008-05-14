/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		Januari 2007
 RCS:		$Id: flatten_seis.cc,v 1.1 2008-05-14 08:53:59 cvsnanne Exp $
________________________________________________________________________

-*/



#include "batchprog.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emsurfaceio.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "plugins.h"
#include "position.h"
#include "ranges.h"
#include "seistrctr.h"
#include "seiszaxisstretcher.h"


static EM::Horizon3D* loadHorizon( const char* id, BufferString& err )
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


#define mErrRet(msg)	{ std::cerr << msg << std::endl; return false; }


bool BatchProgram::go( std::ostream& strm )
{
    PtrMan<IOObj> inioobj = getIOObjFromPars( "Input Seismics", false,
		SeisTrcTranslatorGroup::ioContext(), true );
    PtrMan<IOObj> outioobj = getIOObjFromPars( "Output Seismics", true,
		SeisTrcTranslatorGroup::ioContext(), true );
    if ( !outioobj || !inioobj )
	mErrRet("Need input and output seismics")

    BufferString mid;
    pars().get( "Reference horizon", mid );

    BufferString errmsg;
    EM::Horizon3D* horizon = loadHorizon( mid.buf(), errmsg );
    if ( errmsg != "" ) mErrRet( errmsg )

    float refz; pars().get( "Reference depth", refz );

    CubeSampling cs;
    if ( !cs.hrg.usePar( pars() ) )
	mErrRet( "Cannot read output range" );

    RefMan<EM::HorizonZTransform> zt = new EM::HorizonZTransform( horizon );
    const Interval<float> zrg = zt->getZInterval( false );
    cs.zrg.start = zrg.start;
    cs.zrg.stop = zrg.stop;

    SeisZAxisStretcher exec( *inioobj, *outioobj, cs, *zt, true );
    exec.execute( &strm );
    return true;
}
