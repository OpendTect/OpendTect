/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visnormals.cc,v 1.1 2002-12-20 16:30:27 kristofer Exp $";

#include "visnormals.h"

#include "position.h"
#include "trigonometry.h"
#include "viscoord.h"


#include "Inventor/nodes/SoNormal.h"

mCreateFactoryEntry( visBase::Normals );


visBase::Normals::Normals()
    : coords( 0 )
    , normals( new SoNormal )
{
    normals->ref();
    normaldeps.allowNull();
}


visBase::Normals::~Normals()
{
    normals->unref();
    if ( coords ) coords->unRef();
    deepErase( normaldeps );
}
    

void visBase::Normals::setCoords( Coordinates* newcoords )
{
    if ( coords ) coords->unRef();
    coords = newcoords;
    if ( coords ) coords->ref();
}


int visBase::Normals::addNormal( int p0, int p1, int  p2)
{
    const int res = getFreeIdx();

    while ( normaldeps.size()<=p0 ) normaldeps += 0;
    TypeSet<int>* deps = normaldeps[p0];
    if ( !deps ) { normaldeps.replace( new TypeSet<int>(1,res), p0 ); }
    else { (*deps) += res; }

    while ( normaldeps.size()<=p1 ) normaldeps += 0;
    deps = normaldeps[p1];
    if ( !deps ) { normaldeps.replace( new TypeSet<int>(1,res), p1 ); }
    else { (*deps) += res; }
    
    while ( normaldeps.size()<=p2 ) normaldeps += 0;
    deps = normaldeps[p2];
    if ( !deps ) { normaldeps.replace( new TypeSet<int>(1,res), p2 ); }
    else { (*deps) += res; }

    calcNormal( res, p0, p1, p2 );

    while ( p0s.size()<=res ) p0s += p0;
    while ( p1s.size()<=res ) p1s += p1;
    while ( p2s.size()<=res ) p2s += p2;
    p0s[res] = p0; p1s[res] = p1; p2s[res] = p2;

    return res;
}


void visBase::Normals::removeNormal(int rem)
{
    const int nrdeps = normaldeps.size();
    for ( int idx=0; idx<nrdeps; idx++ )
    {
	TypeSet<int>* deps = normaldeps[0];
	if ( !deps ) continue;
	while ( true )
	{
	    int idy = deps->indexOf( rem );
	    if ( idy==-1 ) break;
	    deps->remove( idy );
	}
    }

    unusednormals += rem;
}


SoNode* visBase::Normals::getData()
{ return normals; }


void visBase::Normals::calcNormal( int nr, int p0, int p1, int p2 )
{
    const Plane3 plane( coords->getPos( p0 ), coords->getPos( p1 ),
	    		coords->getPos( p2 ) );

    Vector3 normal = plane.normal();
    normals->vector.set1Value( nr, SbVec3f( normal.x, normal.y, normal.z ));
}


int  visBase::Normals::getFreeIdx()
{
    if ( unusednormals.size() )
    {
	const int res = unusednormals[unusednormals.size()-1];
	unusednormals.remove(unusednormals.size()-1);
	return res;
    }

    return normals->vector.getNum();
}


void  visBase::Normals::handleCoordChange( CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::CoordinateMessage&,ev,cb);

    if ( ev.event==visBase::CoordinateMessage::ChangedPos )
    {
	TypeSet<int>* deps = ev.coordnr<normaldeps.size()
	    ? normaldeps[ev.coordnr] : 0;

	if ( !deps ) return;
	const int nrnormals = deps->size();

	for ( int idx=0; idx<nrnormals; idx++ )
	{
	    const int normalnr = (*deps)[idx];
	    calcNormal( normalnr, p0s[normalnr], p1s[normalnr], p2s[normalnr] );
	}
    }
    else if ( ev.event==visBase::CoordinateMessage::NrChanged )
    {
	const int oldnr = ev.coordnr;
	const int newnr = ev.newnr;

	const int nrnormals = p0s.size();
	for ( int idx=0; idx<nrnormals; idx++ )
	{
	    if ( p0s[idx]==oldnr ) p0s[idx]=newnr;
	    if ( p1s[idx]==oldnr ) p1s[idx]=newnr;
	    if ( p2s[idx]==oldnr ) p2s[idx]=newnr;
	}

	while ( normaldeps.size()<=newnr ) normaldeps += 0;
	if ( normaldeps[newnr] ) delete  normaldeps[newnr];
	normaldeps.replace( normaldeps[oldnr], newnr );
	normaldeps.replace( 0, oldnr );
    }
}
	





