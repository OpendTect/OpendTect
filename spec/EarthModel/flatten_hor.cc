/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Januari 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: flatten_hor.cc,v 1.4 2010/10/14 09:58:06 cvsbert Exp $";



#include "prog.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emposid.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "position.h"
#include "ranges.h"


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Reference_Horizon_ID Horizon_ID constant [fwd=1]";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static EM::Horizon* loadHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    mDynamicCastGet(EM::Horizon*,horizon,emobj)
    if ( !horizon ) { err = "ID "; err += id; err += " is not horizon"; }
    horizon->ref();
    return horizon;
}


static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();

    BufferString errmsg;
    EM::Horizon* horizon1 = loadHorizon( argv[1], errmsg );
    if ( errmsg != "" ) return prError( errmsg );
    EM::Horizon* horizon2 = loadHorizon( argv[2], errmsg );
    if ( errmsg != "" ) return prError( errmsg );

    const float refz = toFloat( argv[3] );
    const bool forward = argc == 5 ? toInt(argv[4])>0 : true;

    BufferString hornm = horizon2->name();
    hornm += forward ? 	"_flattened" : "_unflattened";
    EM::ObjectID newid = EM::EMM().createObject( EM::Horizon::typeStr(), hornm);
    mDynamicCastGet(EM::Horizon*,newhorizon,EM::EMM().getObject(newid))

    StepInterval<int> inlrg = horizon1->geometry().rowRange();
    StepInterval<int> crlrg = horizon1->geometry().colRange();

    newhorizon->geometry().removeAll();
    const RowCol step( inlrg.step, crlrg.step );
    newhorizon->geometry().setStep( step, step );
    newhorizon->geometry().addSection( 0, false );
    newhorizon->enableGeometryChecks( false );

    EM::PosID posid( newhorizon->id() );
    HorSampling hs; hs.set( inlrg, crlrg );
    HorSamplingIterator iter( hs );
    BinID bid;
    while ( iter.next(bid) )
    {
	const EM::SubID subid = bid.getSerialized();
	for ( int sidx=0; sidx<horizon1->nrSections(); sidx++ )
	{
	    const float z1 = horizon1->getPos( horizon1->sectionID(0), subid).z;
	    const float z2 = horizon2->getPos( horizon2->sectionID(0), subid).z;

	    float newz = mUdf(float);
	    if ( mIsUdf(z1) || mIsUdf(z2) )
		continue;

	    if ( forward )
		newz = z2 - z1 + refz;
	    else
		newz = z2 + z1 - refz;

	    posid.setSectionID( EM::SectionID(sidx) );
	    posid.setSubID( subid );
	    newhorizon->setPos( posid, Coord3(0,0,newz), false );
	}
    }

    std::cerr << "Saving new horizon ..." << std::endl;
    PtrMan<Executor> saver = newhorizon->saver();
    saver->execute();
    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
