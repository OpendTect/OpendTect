/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: gridder2d.cc,v 1.1 2008-03-24 20:15:38 cvskris Exp $";

#include "gridder2d.h"

#include "delaunay.h"
#include "iopar.h"
#include "positionlist.h"
#include "math2.h"
#include "sorting.h"

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

	const double dist = sqrt( sqdist );
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


Gridder2D* TriangulatedGridder2D::create()
{
    return new TriangulatedGridder2D;
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

    Coord2ListImpl coords;
    TypeSet<float> coordweights; //per point
    for ( int idx=0; idx<points_->size(); idx++ )
    {
	const Coord& point = (*points_)[idx];
	if ( !point.isDefined() )
	{
	    coordweights += mUdf(float);
	    continue;
	}

	const float sqdist = gridpoint_.sqDistTo( point );
	if ( mIsZero( sqdist, mEpsilon) )
	{
	    usedvalues_.erase();
	    weights_.erase();
	    usedvalues_ += idx;
	    weights_ += 1;
	    inited_ = true;

	    return true;
	}

	coordweights += 1/Math::Sqrt(sqdist);
	coords.set( idx, point );
    }
    
    const int gridpointid = coords.add( gridpoint_ );

    DelaunayTriangulation dtri( coords );
    const int res = dtri.triangulate();
    double weightsum = 0;
    if ( res == -2 )
	return false;
    else if ( !res )
    {
	pErrMsg("Should never happen");
	return false;
    }
    else if ( res==-1 )
    {
	TypeSet<int> coordindices;
	for ( int idx=0; idx<coordweights.size(); idx++ )
	    coordindices += idx;

	for ( int idx=coordweights.size()-1; idx>=0; idx-- )
	{
	    if ( !mIsUdf(coordweights[idx]) )
		continue;

	    coordweights.remove( idx );
	    coordindices.remove( idx );
	}

	const int nrcoords = coordindices.size();

	sort_coupled( coordweights.arr(), coordindices.arr(), nrcoords );

	//Always include closest pt
	const int closestidx = coordindices[nrcoords-1];

	usedvalues_ += closestidx;
	weights_ += coordweights[nrcoords-1];
	weightsum = weights_[0];

	const Coord v1 = (*points_)[closestidx] - gridpoint_;

	//Search for closest pt not on the same side as first pt.
	for ( int idx=nrcoords-2; idx>=0; idx-- )
	{
	    const Coord v2 = (*points_)[coordindices[idx]] - gridpoint_;
	    if ( v1.dot( v2 )<0 )
	    {
		usedvalues_ += coordindices[idx];
		weights_ += coordweights[idx];
		weightsum += coordweights[idx];
		break;
	    }
	}
    }
    else
    {
	const TypeSet<int>& indices = dtri.getCoordIndices();
	const int nrindices = indices.size();

	int iidx = 0;
	while ( iidx<nrindices )
	{
	    iidx = indices.indexOf( gridpointid, true, iidx );
	    if ( iidx==-1 )
		break;

	    const int triangle = iidx/3;
#define mAddCoordIndex( offset ) \
{ \
    const int ci = indices[triangle*3+offset]; \
    const int ciidx = usedvalues_.indexOf( ci ); \
    if ( ciidx==-1 ) \
    { \
	usedvalues_ += ci; \
	const double weight = coordweights[ci]; \
	weights_ += weight; \
	weightsum += weight; \
    } \
}

	    mAddCoordIndex( 0 );
	    mAddCoordIndex( 1 );
	    mAddCoordIndex( 2 );

	    iidx++;
	}
    }

    for ( int idx=weights_.size()-1; idx>=0; idx-- )
        weights_[idx] /= weightsum;

    inited_ = true;
    return true;
}
