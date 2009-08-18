/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: gridder2d.cc,v 1.21 2009-08-18 21:29:27 cvsyuancheng Exp $";

#include "gridder2d.h"

#include "delaunay.h"
#include "iopar.h"
#include "positionlist.h"
#include "math2.h"
#include "sorting.h"
#include "trigonometry.h"

#define mEpsilon 0.0001

mImplFactory( Gridder2D, Gridder2D::factory );


Gridder2D::Gridder2D()
    : values_( 0 )
    , points_( 0 )
    , inited_( false )
{}


Gridder2D::Gridder2D( const Gridder2D & )
    : values_( 0 )
    , points_( 0 )
    , inited_( false )
{}


bool Gridder2D::operator==( const Gridder2D& b ) const
{
    return name()==b.name();
}


bool Gridder2D::isPointUsable(const Coord& cpt,const Coord& dpt) const
{ return true; }


bool Gridder2D::setPoints( const TypeSet<Coord>& cl )
{
    points_ = &cl;
    inited_ = false;
    return true;
}


bool Gridder2D::setValues( const TypeSet<float>& vl, bool hasudfs )
{
    values_ = &vl;
    if ( hasudfs ) inited_ = false;
    return true;
}


bool Gridder2D::setGridPoint( const Coord& pt )
{
    inited_ = false;
    gridpoint_ = pt;
    return gridpoint_.isDefined();
}


float Gridder2D::getValue() const
{
    if ( !inited_ )
	return mUdf(float);

    if ( !usedvalues_.size() )
	return mUdf(float);

    if ( !values_ )
	return mUdf(float);;

    double valweightsum = 0;
    double weightsum = 0;
    int nrvals = 0;
    const int nrvalues = values_->size();
    for ( int idx=usedvalues_.size()-1; idx>=0; idx-- )
    {
	const int validx = usedvalues_[idx];
	if ( validx>=nrvalues )
	{
	    pErrMsg("Should not happen");
	    return mUdf(float);
	}

	if ( mIsUdf((*values_)[validx]) )
	    continue;

	valweightsum += (*values_)[validx] * weights_[idx];
	weightsum += weights_[idx];
	nrvals++;
    }

    if ( !nrvals )
	return mUdf(float);

    if ( nrvals==usedvalues_.size() )
	return valweightsum;

    return valweightsum/weightsum;
}


bool Gridder2D::isPointUsed( int idx ) const
{
    return usedvalues_.indexOf(idx) != -1;
}


InverseDistanceGridder2D::InverseDistanceGridder2D()
    : radius_( mUdf(float) )
{}


InverseDistanceGridder2D::InverseDistanceGridder2D(
	const InverseDistanceGridder2D& g )
    : radius_( g.radius_ )
{}


Gridder2D* InverseDistanceGridder2D::create()
{
    return new InverseDistanceGridder2D;
}


void InverseDistanceGridder2D::initClass()
{
    Gridder2D::factory().addCreator( create, sName(), sUserName() );
}


Gridder2D* InverseDistanceGridder2D::clone() const
{ return new InverseDistanceGridder2D( *this ); }


bool InverseDistanceGridder2D::operator==( const Gridder2D& b ) const
{
    if ( !Gridder2D::operator==( b ) )
	return false;

    mDynamicCastGet( const InverseDistanceGridder2D*, bidg, &b );
    if ( !bidg )
	return false;

    if ( mIsUdf(radius_) && mIsUdf( bidg->radius_ ) )
	return true;

    return mIsEqual(radius_,bidg->radius_, 1e-5 );
}


void InverseDistanceGridder2D::setSearchRadius( float r )
{
    if ( r<= 0 )
	return;

    radius_ = r;
}


bool InverseDistanceGridder2D::isPointUsable( const Coord& calcpt,
					      const Coord& datapt ) const
{
    if ( !datapt.isDefined() || !calcpt.isDefined() )
	return false;

    return mIsUdf(radius_) || calcpt.sqDistTo( datapt ) < radius_*radius_;
}


bool InverseDistanceGridder2D::init()
{
    usedvalues_.erase();
    weights_.erase();

    if ( !gridpoint_.isDefined() || !points_ || !points_->size() )
	return false;

    const bool useradius = !mIsUdf(radius_);
    const double radius2 = radius_*radius_;


    double weightsum = 0;
    for ( int idx=points_->size()-1; idx>=0; idx-- )
    {
	if ( !(*points_)[idx].isDefined() )
	    continue;

	const double sqdist = gridpoint_.sqDistTo( (*points_)[idx] );
	if ( useradius && sqdist>radius2 )
	    continue;

	if ( mIsZero(sqdist, mEpsilon) )
	{
	    usedvalues_.erase();
	    weights_.erase();

	    usedvalues_ += idx;
	    weights_ += 1;
	    inited_ = true;
	    return true;
	}

	const double dist = Math::Sqrt( sqdist );
	const double weight = 1/dist;

	weightsum += weight;
	weights_ += weight;
	usedvalues_ += idx;
    }

    for ( int idx=weights_.size()-1; idx>=0; idx-- )
	weights_[idx] /= weightsum;

    inited_ = true;
    return true;
}


bool InverseDistanceGridder2D::usePar( const IOPar& par )
{
    float r;
    if ( !par.get( sKeySearchRadius(), r ) )
	return false;

    setSearchRadius( r ); 
    return true;
}


void InverseDistanceGridder2D::fillPar( IOPar& par ) const
{
    par.set( sKeySearchRadius(), getSearchRadius() );
}


TriangulatedNeighborhoodGridder2D::TriangulatedNeighborhoodGridder2D()
    : triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( mUdf(float), mUdf(float) )
    , yrg_( mUdf(float), mUdf(float) )
{}


TriangulatedNeighborhoodGridder2D::TriangulatedNeighborhoodGridder2D(
	const TriangulatedNeighborhoodGridder2D& b )
    : triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( b.xrg_ )
    , yrg_( b.yrg_ )
{
    if ( b.triangles_ )
    {
	triangles_ = new DAGTriangleTree( *b.triangles_ );
	interpolator_ = new Triangle2DInterpolator( *triangles_ );
    }
}


TriangulatedNeighborhoodGridder2D::~TriangulatedNeighborhoodGridder2D()
{ 
    delete triangles_; 
    delete interpolator_;
}


void TriangulatedNeighborhoodGridder2D::setGridArea( const Interval<float>& xrg,
						     const Interval<float>& yrg)
{
    xrg_ = xrg;
    yrg_ = yrg;
}


Gridder2D* TriangulatedNeighborhoodGridder2D::create()
{
    return new TriangulatedNeighborhoodGridder2D;
}


void TriangulatedNeighborhoodGridder2D::initClass()
{
    Gridder2D::factory().addCreator( create, sName(), sUserName() );
}


Gridder2D* TriangulatedNeighborhoodGridder2D::clone() const
{ return new TriangulatedNeighborhoodGridder2D( *this ); }


bool TriangulatedNeighborhoodGridder2D::init()
{
    usedvalues_.erase();
    weights_.erase();

    inited_ = false;

    if ( !points_ || !points_->size() || !gridpoint_.isDefined() )
	return true;

    if ( points_->size()==1 )
    {
	usedvalues_ += 0;
	weights_ += 1;
	inited_ = true;
	return true;
    }

    if ( !triangles_ )
    {
	triangles_ = new DAGTriangleTree;
	Interval<double> xrg, yrg;
	if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) ) 
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}

	TypeSet<Coord>* pts = new TypeSet<Coord>;
	for ( int idx=0; idx<points_->size(); idx++ )
	    (*pts) += (*points_)[idx];

	if ( !triangles_->setCoordList( pts, OD::TakeOverPtr ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}

	if ( !mIsUdf(xrg_.start) ) xrg.include( xrg_.start );
	if ( !mIsUdf(xrg_.stop) ) xrg.include( xrg_.stop );
	if ( !mIsUdf(yrg_.start) ) yrg.include( yrg_.start );
	if ( !mIsUdf(yrg_.stop) ) yrg.include( yrg_.stop );

	if ( !triangles_->setBBox( xrg, yrg ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}

	ParallelDTriangulator triangulator( *triangles_ );
	triangulator.dataIsRandom( false );
	if ( !triangulator.execute( false ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}
    
	if ( interpolator_ )
    	    delete interpolator_;
	
    	interpolator_ = new Triangle2DInterpolator( *triangles_ );
    }

    if ( !interpolator_->computeWeights(gridpoint_,usedvalues_,weights_) )
	return false;

    inited_ = true;
    return true;
}


TriangulatedGridder2D::TriangulatedGridder2D()
    : triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( mUdf(float), mUdf(float) )
    , yrg_( mUdf(float), mUdf(float) )
{}


TriangulatedGridder2D::TriangulatedGridder2D( 
				const TriangulatedGridder2D& b )
    : triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( b.xrg_ )
    , yrg_( b.yrg_ )
{
    if ( b.triangles_ )
    {
	triangles_ = new DAGTriangleTree( *b.triangles_ );
	interpolator_ = new Triangle2DInterpolator( *triangles_ );
    }
}


TriangulatedGridder2D::~TriangulatedGridder2D()
{ 
    delete triangles_; 
    delete interpolator_;
}


Gridder2D* TriangulatedGridder2D::create()
{
    return new TriangulatedGridder2D;
}


void TriangulatedGridder2D::setGridArea( const Interval<float>& xrg,
					 const Interval<float>& yrg )
{
    xrg_ = xrg;
    yrg_ = yrg;
}


void TriangulatedGridder2D::initClass()
{
    Gridder2D::factory().addCreator( create, sName(), sUserName() );
}


Gridder2D* TriangulatedGridder2D::clone() const
{ return new TriangulatedGridder2D( *this ); }


bool TriangulatedGridder2D::init()
{
    usedvalues_.erase();
    weights_.erase();

    inited_ = false;

    if ( !points_ || !points_->size() || !gridpoint_.isDefined() )
	return true;

    if ( points_->size()==1 )
    {
	usedvalues_ += 0;
	weights_ += 1;
	inited_ = true;
	return true;
    }

    if ( !triangles_ )
    {
	triangles_ = new DAGTriangleTree;
	Interval<double> xrg, yrg;
	if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}

	if ( !mIsUdf(xrg_.start) ) xrg.include( xrg_.start );
	if ( !mIsUdf(xrg_.stop) ) xrg.include( xrg_.stop );
	if ( !mIsUdf(yrg_.start) ) yrg.include( yrg_.start );
	if ( !mIsUdf(yrg_.stop) ) yrg.include( yrg_.stop );

	TypeSet<Coord>* pts = new TypeSet<Coord>;
	for ( int idx=0; idx<points_->size(); idx++ )
	    (*pts) += (*points_)[idx];

	const float radius = Math::Sqrt( xrg.width()*xrg.width() + 
				   yrg.width()*yrg.width() )/2*1.05;
	const int nrptsinsert = 10; //should be >2.
	for ( int idx=0; idx<nrptsinsert/2; idx++ )
	{
	    const double length = radius*((float)idx*4/(nrptsinsert-2)-1);
	    const double x = xrg.center()+length;
	    const double y = Math::Sqrt( radius*radius-length*length ); 
	    (*pts) += Coord( x, yrg.center()+y );
	    if ( idx && idx<nrptsinsert/2-1 )
		(*pts) += Coord( x, yrg.center()-y );
	}

	addedindices_.erase();
    	for ( int idx=pts->size()-1; idx>=points_->size(); idx-- )
    	    addedindices_ += idx;

	if ( !triangles_->setCoordList( pts, OD::TakeOverPtr ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}
	
	ParallelDTriangulator triangulator( *triangles_ );
	triangulator.dataIsRandom( false ); 
	if ( !triangulator.execute( false ) )
	{
	    delete triangles_;
	    triangles_ = 0;
	    return false;
	}
    
	if ( interpolator_ )
    	    delete interpolator_;
	
	interpolator_ = new Triangle2DInterpolator( *triangles_ );
    }

    TypeSet<int> vertices;
    TypeSet<float> weight;
    if ( !interpolator_->computeWeights(gridpoint_,vertices,weight) )
	return false;

    for ( int idx=0; idx<vertices.size(); idx++ )
    {
	if ( vertices[idx]>=points_->size() )
	{
	    TypeSet<int> conns;
	    TypeSet<double> ws;
	    triangles_->getConnectionAndWeights(vertices[idx],conns,ws,false);
	    
	    double weightsum = 0;
	    for ( int idy=0; idy<ws.size(); idy++ )
	    {
		if ( addedindices_.indexOf(conns[idy])!=-1 )
		{
		    ws.remove( idy );
		    conns.remove( idy );
		    idy--;
		    continue;
		}
		
		weightsum += ws[idy];
	    }
	    
	    for ( int idy=0; idy<ws.size(); idy++ )
	    {
		const int ptidx = usedvalues_.indexOf( conns[idy] );
		if ( ptidx==-1 )
		{
		    usedvalues_ += conns[idy];
		    weights_ += weight[idx]*ws[idy]/weightsum;
		}
		else
		{
		    weights_[ptidx] += weight[idx]*ws[idy]/weightsum;
		}
	    }
	}
	else
	{
	    const int ptidx = usedvalues_.indexOf( vertices[idx] );
	    if ( ptidx==-1 )
	    {
		usedvalues_ += vertices[idx];
		weights_ += weight[idx];
	    }
	    else
		weights_[ptidx] += weight[idx];
	}
    }

    inited_ = true;
    return true;
}


