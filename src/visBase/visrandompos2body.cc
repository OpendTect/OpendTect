/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          January 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "visrandompos2body.h"

#include "delaunay3d.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "viscoord.h"
#include "vistransform.h"
#include "vistristripset.h"

#include <Inventor/nodes/SoShapeHints.h>

mCreateFactoryEntry( visBase::RandomPos2Body );

namespace visBase
{

RandomPos2Body::RandomPos2Body()
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , triset_( 0 )
    , hints_( new SoShapeHints )		  
{
    addChild( hints_ );
    renderOneSide( 0 );
}


RandomPos2Body::~RandomPos2Body()
{
    if ( transformation_ ) transformation_->unRef();
    if ( triset_ ) 
    {
	removeChild( triset_->getInventorNode() );
	triset_->unRef();
    }
}


void RandomPos2Body::renderOneSide( int side )
{
    hints_->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    
    if ( side==0 )
    {
	hints_->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    }
    else if ( side==1 )
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
    else
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
}

bool RandomPos2Body::setPoints( const TypeSet<Coord3>& pts )
{
    picks_.erase();
    picks_.copy( pts );

    if ( !pts.size() )
	return true;

    if ( !triset_ )
    {
	triset_ = visBase::TriangleStripSet::create();
	triset_->ref();
	triset_->setDisplayTransformation( transformation_ );
	addChild( triset_->getInventorNode() );
    }

    TypeSet<Coord3> picks;
    const float zscale = SI().zFactor();
    bool oninline = true, oncrossline = true, onzslice = true;
    int inl=-1, crl=-1;
    float z=-1;
    const float zeps = SI().zStep()/2.0;

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	triset_->getCoordinates()->setPos( idx, pts[idx] );
	picks += Coord3(pts[idx].coord(), pts[idx].z*zscale);

	const BinID bid = SI().transform( pts[idx] );
	if ( idx==0 )
	{
	    inl = bid.inl;
	    crl = bid.crl;
	    z = pts[idx].z;
	}
	else
	{
	    if ( oninline && inl!=bid.inl )
		oninline = false;
	    
	    if ( oncrossline && crl!=bid.crl )
		oncrossline = false;
	    
	    if ( onzslice && !mIsEqual(z,pts[idx].z,zeps) )
		onzslice = false;
	}
    }

    triset_->getCoordinates()->removeAfter( pts.size()-1 );

    TypeSet<int> result;
    if ( !oninline && !oncrossline && !onzslice )
    {
     	DAGTetrahedraTree tree;
    	tree.setCoordList( picks, false );
    	
    	ParallelDTetrahedralator pdtri( tree );
    	if ( !pdtri.execute(true) )
    	    return false;
    
	if ( !tree.getSurfaceTriangles(result) )
    	    return false;
    }
    else
    {
	TypeSet<Coord> knots;
	if ( onzslice )
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += pts[idx].coord();
	}
	else if ( oninline )
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += Coord(picks[idx].y,picks[idx].z);
	}
	else
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += Coord(picks[idx].x,picks[idx].z);
	}

	polygonTriangulate( knots, result );
    }

    int cii = 0;
    for ( int idx=0; idx<result.size()/3; idx++ )
    {
	triset_->setCoordIndex( cii++, result[3*idx] );
	triset_->setCoordIndex( cii++, result[3*idx+1] );
	triset_->setCoordIndex( cii++, result[3*idx+2] );
	triset_->setCoordIndex( cii++, -1 );
    }
    
    triset_->removeCoordIndexAfter( cii-1 );
    
    return true;
}


void RandomPos2Body::setDisplayTransformation( const mVisTrans*  nt )
{
    if ( transformation_ ) transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ ) 
    {
	transformation_->ref();
    	if ( triset_ )
    	    triset_->setDisplayTransformation( transformation_ );
    }
}


const mVisTrans* RandomPos2Body::getDisplayTransformation() const
{ return transformation_; }


void RandomPos2Body::polygonTriangulate( const TypeSet<Coord>& knots,
					 TypeSet<int>& res )
{
    const int nrknots = knots.size();
    if ( nrknots < 3 ) return;

    double area=0;
    for( int idx=nrknots-1, idy=0; idy<nrknots; idx=idy++ )
	area += (knots[idx].x*knots[idy].y - knots[idy].x*knots[idx].y);
    area *= 0.5;
    
    TypeSet<int> ci;
    if ( 0<area )
    {
	for ( int idx=0; idx<nrknots; idx++ )
	    ci += idx;
    }
    else
    {
	for( int idx=0; idx<nrknots; idx++ )
	    ci += (nrknots-1-idx);
    }
    
    int cursize = nrknots;
    int errcheck = 2*cursize;
    
    for( int idx=cursize-1; cursize>2; )
    {
	if ( 0 >= (errcheck--) )
	    return;
	
	const int idx0 = cursize<=idx ? 0 : idx;
	idx = cursize<=idx0+1 ? 0 : idx0+1;
	const int idx1 = cursize<=idx+1 ? 0 : idx+1;
	
	const Coord& pos0 = knots[ci[idx0]];
	const Coord& pos = knots[ci[idx]];
	const Coord& pos1 = knots[ci[idx1]];
	if ( (((pos.x-pos0.x)*(pos1.y-pos0.y)) -
	      ((pos.y-pos0.y)*(pos1.x-pos0.x)))<0 )
	    continue;
	
	bool isvalid = true;
	for ( int idy=0; idy<cursize; idy++ )
	{
	    if( (idy==idx0) || (idy==idx) || (idy==idx1) )
		continue;
	    
	    if ( pointInTriangle2D(knots[ci[idy]],pos0,pos,pos1,0.0) )
	    {
		isvalid = false;
		break;
	    }
	}

	if ( isvalid )
	{
	    res += ci[idx0];
	    res += ci[idx];
	    res += ci[idx1];
	    
	    for( int i=idx, j=idx+1; j<cursize; i++, j++ )
		ci[i] = ci[j];
	    
	    cursize--;
	    errcheck = 2*cursize;
	}
    }
}


}; // namespace visBase
