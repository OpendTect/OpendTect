/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

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


Gridder2D::Gridder2D( const Gridder2D& g )
    : values_( g.values_ )
    , points_( g.points_ )
    , inited_( false )
{}


bool Gridder2D::operator==( const Gridder2D& b ) const
{
    return factoryKeyword()==b.factoryKeyword();
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
	return (float) valweightsum;

    return (float) ( valweightsum/weightsum );
}


bool Gridder2D::isPointUsed( int idx ) const
{
    return usedvalues_.isPresent(idx);
}


InverseDistanceGridder2D::InverseDistanceGridder2D()
    : radius_( mUdf(float) )
{}


InverseDistanceGridder2D::InverseDistanceGridder2D(
	const InverseDistanceGridder2D& g )
    : radius_( g.radius_ )
{}


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
	const double weight = useradius ? 1-dist/radius_ : 1./dist;

	weightsum += weight;
	weights_ += (float) weight;
	usedvalues_ += idx;
    }

    for ( int idx=weights_.size()-1; idx>=0; idx-- )
	weights_[idx] /= (float) weightsum;

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

TriangulatedGridder2D::TriangulatedGridder2D()
    : triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( mUdf(float), mUdf(float) )
    , yrg_( mUdf(float), mUdf(float) )
    , center_( 0, 0 )
{}


TriangulatedGridder2D::TriangulatedGridder2D( 
				const TriangulatedGridder2D& b )
    : Gridder2D( b )
    , triangles_( 0 )
    , interpolator_( 0 )  
    , xrg_( b.xrg_ )
    , yrg_( b.yrg_ )
    , center_( b.center_ )
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


void TriangulatedGridder2D::setGridArea( const Interval<float>& xrg,
					 const Interval<float>& yrg )
{
    xrg_ = xrg;
    yrg_ = yrg;
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

    Interval<double> xrg, yrg;
    if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) )
	return false;
    
    if ( !triangles_ || !xrg.includes(gridpoint_.x,false) ||
			!yrg.includes(gridpoint_.y,false) )
    {
	float weightsum = 0;
	for ( int idx=points_->size()-1; idx>=0; idx-- )
	{
	    if ( !(*points_)[idx].isDefined() )
		continue;
	    
	    const double dist = gridpoint_.distTo( (*points_)[idx] );
	    if ( mIsZero(dist,mEpsilon) )
	    {
		usedvalues_.erase();
		weights_.erase();
		usedvalues_ += idx;
		weights_ += 1;
		inited_ = true;
		return true;
	    }
	    
	    const float weight = 1./dist;
	    weightsum += weight;
	    weights_ += weight;
	    usedvalues_ += idx;
	}

	if ( !weights_.size() )
	    return false;
	
	for ( int idx=weights_.size()-1; idx>=0; idx-- )
	    weights_[idx] /= weightsum;
    }
    else
    {
	if ( !interpolator_->computeWeights(
		    gridpoint_-center_, usedvalues_, weights_ ) )
	    return false;
    }

    inited_ = true;
    return true;
}


bool TriangulatedGridder2D::setPoints( const TypeSet<Coord>& pts )
{
    if ( !Gridder2D::setPoints(pts) )
	return false;

    delete triangles_;
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

    TypeSet<Coord>* translatedpoints =
	new TypeSet<Coord>( points_->size(), Coord::udf() );

    center_.x = xrg.center();
    center_.y = yrg.center();

    for ( int idx=points_->size()-1; idx>=0; idx-- )
	(*translatedpoints)[idx] = (*points_)[idx]-center_;

    if ( !triangles_->setCoordList( translatedpoints, OD::TakeOverPtr ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    xrg.start -= center_.x;
    xrg.stop -= center_.x;
    yrg.start -= center_.y;
    yrg.stop -= center_.y;

    if ( !triangles_->setBBox( xrg, yrg ) )
    {
	delete triangles_;
 	triangles_ = 0;
	return false;
    }
    
    DelaunayTriangulator triangulator( *triangles_ );
    triangulator.dataIsRandom( false );
    if ( !triangulator.execute( false ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    delete interpolator_;
    
    interpolator_ = new Triangle2DInterpolator( *triangles_ );

    return true;
}
