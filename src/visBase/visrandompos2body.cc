/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          January 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visrandompos2body.h"

#include "delaunay3d.h"
#include "survinfo.h"
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
    const float zscale = SI().zDomain().userFactor();
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	triset_->getCoordinates()->setPos( idx, pts[idx] );
	picks += Coord3(pts[idx].coord(), pts[idx].z*zscale);
    }

    triset_->getCoordinates()->removeAfter( pts.size()-1 );

    DAGTetrahedraTree tree;
    tree.setCoordList( picks, false );
    
    ParallelDTetrahedralator pdtri( tree );
    if ( !pdtri.execute(true) )
	return false;

    TypeSet<int> result;
    if ( !tree.getSurfaceTriangles(result) )
	return false;

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


}; // namespace visBase
