/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.2 2002-05-21 09:46:36 kristofer Exp $";

#include "emhorizon.h"
#include "geomcompositesurface.h"
#include "geomtristripset.h"
#include "emhorizontransl.h"
#include "executor.h"

#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"

EarthModel::Horizon::Horizon(EMManager& man, int id_)
    : EMObject( man, id_ )
    , surfaces( *new Geometry::CompositeGridSurface )
{ }


EarthModel::Horizon::~Horizon()
{
    delete &surfaces;
}


int EarthModel::Horizon::findPos( int inl, int crl, TypeSet<PosID>& res ) const
{
    res.erase();

    const int nrsubsurf = surfaces.nrSubSurfaces();
    for ( unsigned short surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridNode gridnode(inl,crl);
	Geometry::Pos pos = surfaces.getPos( surface, gridnode );
	if ( !pos.isDefined() ) continue;

	unsigned long surfpid = Geometry::GridSurface::getPosId( gridnode );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( surfpid!=getSurfPID(res[idx]) ) continue;

	    unsigned short tmpsurf = getSurfID(res[idx]);
	    Geometry::Pos tmppos = surfaces.getPos( tmpsurf, surfpid );
	    if ( mIS_ZERO( tmppos.z-pos.z ) ) continue;

	    PosID pid = getPosID( surface, surfpid );
	    res += pid;
	}
    }

    return res.size();
}


void EarthModel::Horizon::addSquare( int inl, int crl,
				     float inl0crl0z, float inl0crl1z,
				     float inl1crl0z, float inl1crl1z )
{
    surfaces.addSquare( Geometry::GridNode( inl, crl ),
	    			inl0crl0z, inl0crl1z, inl1crl0z, inl1crl1z );
}


unsigned short EarthModel::Horizon::getSurfID( PosID posid )
{
    return (posid.id>>32) & 0x0000FFFF;
}


unsigned long EarthModel::Horizon::getSurfPID( PosID posid )
{
    return posid.id & 0x00000000FFFFFFFFl;
}


EarthModel::PosID EarthModel::Horizon::getPosID( unsigned short surfid,
						 unsigned long  surfpid ) const
{
    PosID res;
    res.id = ( ((unsigned long long) id())<<48 ) +
	     ( ((unsigned long long) surfid)<<32 ) +
	     surfpid;
    return res;
}


Executor* EarthModel::Horizon::loader()
{
    MultiID key = IOM().key();
    key.add( id() );
    PtrMan<IOObj> ioobj = IOM().get( key );

    Executor* exec = EarthModelHorizonTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


Executor* EarthModel::Horizon::saver()
{
    MultiID key = IOM().key();
    key.add( id() );
    PtrMan<IOObj> ioobj = IOM().get( key );

    Executor* exec = EarthModelHorizonTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}



void EarthModel::Horizon::getTriStrips(
				Geometry::TriangleStripSet* tristrips ) const
{
}
