/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.1 2002-05-16 14:18:55 kristofer Exp $";

#include "emhorizon.h"
#include "geom2dsnappedsurface.h"
#include "geomtristripset.h"
#include "emhorizontransl.h"
#include "executor.h"

#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"

EarthModel::Horizon::Horizon(EMManager& man, int id_)
    : EMObject( man, id_ )
{
    surfaces += new Geometry::Snapped2DSurface;
}


EarthModel::Horizon::~Horizon()
{
    deepErase( surfaces );
}


int EarthModel::Horizon::findPos( int inl, int crl, TypeSet<PosID>& res ) const
{
    res.erase();

    for ( unsigned short surface=0; surface<surfaces.size(); surface++ )
    {
	Geometry::GridNode gridnode(inl,crl);
	Geometry::Pos pos = surfaces[surface]->getPos( gridnode );
	if ( !pos.isDefined() ) continue;

	unsigned long surfpid = Geometry::GridSurface::getPosId( gridnode );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( surfpid!=getSurfPID(res[idx]) ) continue;

	    unsigned short tmpsurf = getSurfID(res[idx]);
	    Geometry::Pos tmppos = surfaces[tmpsurf]->getPos( surfpid );
	    if ( mIS_ZERO( tmppos.z-pos.z ) ) continue;

	    PosID pid = getPosID( surface, surfpid );
	    res += pid;
	}
    }

    return res.size();
}


void EarthModel::Horizon::addSquare( int inl, int crl,
				     float inl0crl0z, float inl0crl1z,
				     float inl1crl0z, float inl1crl1z,
				     FillType ft )
{
    Geometry::GridNode inl0crl0node(inl,crl);
    Geometry::GridNode inl0crl1node(inl,crl+1);
    Geometry::GridNode inl1crl0node(inl+1,crl);
    Geometry::GridNode inl1crl1node(inl+1,crl+1);

    // Is the square already defined ?
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	if ( surfaces[idx]->isDefined( inl0crl0node ))
	{
	    if ( !mIS_ZERO( surfaces[idx]->getPos(inl0crl0node).z-inl0crl0z ))
		continue;
	    if ( !mIS_ZERO( surfaces[idx]->getPos(inl0crl1node).z-inl0crl1z ))
		continue;
	    if ( !mIS_ZERO( surfaces[idx]->getPos(inl1crl0node).z-inl1crl0z ))
		continue;
	    if ( !mIS_ZERO( surfaces[idx]->getPos( inl1crl1node).z-inl1crl1z ))
		continue;
	    
	    return;
	}
    }

    // Determine which pos are aready defined and on what subsurfaces

    TypeSet<int>	inl0crl0s;
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	if ( mIS_ZERO( surfaces[idx]->getPos(inl0crl0node).z - inl0crl0z) )
	    inl0crl0s += idx;
    }

    TypeSet<int>	inl0crl1s;
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	if ( mIS_ZERO( surfaces[idx]->getPos(inl0crl1node).z - inl0crl1z) )
	    inl0crl1s += idx;
    }

    TypeSet<int>	inl1crl0s;
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	if ( mIS_ZERO( surfaces[idx]->getPos(inl1crl0node).z - inl1crl0z) )
	    inl1crl0s += idx;
    }

    TypeSet<int>	inl1crl1s;
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	if ( mIS_ZERO( surfaces[idx]->getPos(inl1crl1node).z - inl1crl1z) )
	    inl1crl1s += idx;
    }


    // Determine what subsurface we are going to place it on.
    int subsurf = -1;

    if ( inl0crl0s.size() ) subsurf = inl0crl0s[0];
    else if ( inl0crl1s.size() ) subsurf = inl0crl0s[0];
    else if ( inl1crl0s.size() ) subsurf = inl1crl0s[0];
    else if ( inl1crl1s.size() ) subsurf = inl1crl1s[0];

    if ( subsurf==-1 )
    {
	if ( surfaces.size() ) return;

	surfaces += new Geometry::Snapped2DSurface;
	//TODO: Set transform
	subsurf = 0;
    }


    Geometry::Snapped2DSurface* surf = surfaces[subsurf];

    if ( inl0crl0s.indexOf(subsurf) == -1 )
    {
	Geometry::Pos pos;
	pos.z = inl0crl0z;
	surf->setPos( inl0crl0node, pos );
	unsigned long pid = Geometry::GridSurface::getPosId( inl0crl0node );
	for ( int idx=0; inl0crl0s.size(); idx++ )
	    surf->setLink( pid, surfaces[inl0crl0s[idx]], pid );
    }

    if ( inl0crl1s.indexOf(subsurf) == -1 )
    {
	Geometry::Pos pos;
	pos.z = inl0crl1z;
	surf->setPos( inl0crl1node, pos );
	unsigned long pid = Geometry::GridSurface::getPosId( inl0crl1node );
	for ( int idx=0; inl0crl1s.size(); idx++ )
	    surf->setLink( pid, surfaces[inl0crl1s[idx]], pid );
    }

    if ( inl1crl0s.indexOf(subsurf) == -1 )
    {
	Geometry::Pos pos;
	pos.z = inl1crl0z;
	surf->setPos( inl1crl0node, pos );
	unsigned long pid = Geometry::GridSurface::getPosId( inl1crl0node );
	for ( int idx=0; inl1crl0s.size(); idx++ )
	    surf->setLink( pid, surfaces[inl1crl0s[idx]], pid );
    }

    if ( inl1crl1s.indexOf(subsurf) == -1 )
    {
	Geometry::Pos pos;
	pos.z = inl1crl1z;
	surf->setPos( inl1crl1node, pos );
	unsigned long pid = Geometry::GridSurface::getPosId( inl1crl1node );
	for ( int idx=0; inl1crl1s.size(); idx++ )
	    surf->setLink( pid, surfaces[inl1crl1s[idx]], pid );
    }

    surf->setFillType( inl0crl0node, Geometry::GridSurface::Filled );
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
    tristrips->removeAll();
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	surfaces[idx]->fillTriStipSet( tristrips );
    }
}
