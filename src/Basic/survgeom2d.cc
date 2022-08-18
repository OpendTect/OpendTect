/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "survgeom2d.h"

#include "posinfo2d.h"


#define mSetSampling \
const StepInterval<Pos::TraceID> trcrg = data_.trcNrRange(); \
sampling_.zsamp_ = data_.zRange(); \
sampling_.hsamp_.start_.lineNr() = sampling_.hsamp_.stop_.lineNr() \
				 = getID().asInt(); \
sampling_.hsamp_.start_.trcNr() = trcrg.start; \
sampling_.hsamp_.stop_.trcNr() = trcrg.stop; \
sampling_.hsamp_.step_.trcNr() = trcrg.step

Survey::Geometry2D::Geometry2D( const char* lnm )
    : data_(*new PosInfo::Line2DData(lnm))
    , trcdist_(mUdf(float))
    , linelength_(mUdf(float))
{
    sampling_.hsamp_.survid_ = OD::Geom2D;
    mSetSampling;
}


Survey::Geometry2D::Geometry2D( PosInfo::Line2DData* l2d )
    : data_(l2d ? *l2d : *new PosInfo::Line2DData)
    , trcdist_(mUdf(float))
    , linelength_(mUdf(float))
{
    sampling_.hsamp_.survid_ = OD::Geom2D;
    mSetSampling;
}


Survey::Geometry2D::~Geometry2D()
{ delete &data_; }


void Survey::Geometry2D::add( const Coord& crd, int trcnr, float spnr )
{
    add( crd.x, crd.y, trcnr, spnr );
}


void Survey::Geometry2D::add( double x, double y, int trcnr, float spnr )
{
    PosInfo::Line2DPos pos( trcnr );
    pos.coord_.x = x;
    pos.coord_.y = y;
    data_.add( pos );
    spnrs_ += spnr;
}


int Survey::Geometry2D::size() const
{
    return data_.size();
}


bool Survey::Geometry2D::isEmpty() const
{
    return data_.isEmpty();
}


void Survey::Geometry2D::setEmpty()
{
    data_.setEmpty();
    spnrs_.erase();
}


bool Survey::Geometry2D::getPosByTrcNr( int trcnr, Coord& crd,
					float& spnr ) const
{
    const int posidx = data_.indexOf( trcnr );
    if ( !data_.positions().validIdx(posidx) )
	return false;

    crd = data_.positions()[posidx].coord_;
    spnr = spnrs_.validIdx(posidx) ? spnrs_[posidx] : mUdf(int);
    return true;
}


bool Survey::Geometry2D::getPosBySPNr( float spnr, Coord& crd, int& trcnr) const
{
    int posidx=-1;
    for ( int idx=0; idx<spnrs_.size(); idx++ )
    {
	if ( !mIsEqual(spnr,spnrs_[idx],0.001) )
	    continue;

	posidx = idx;
	break;
    }

    if ( !data_.positions().validIdx(posidx) )
	return false;

    crd = data_.positions()[posidx].coord_;
    trcnr = data_.positions()[posidx].nr_;
    return true;
}


bool Survey::Geometry2D::getPosByCoord( const Coord& crd, int& trcnr,
					float& spnr ) const
{
    const int posidx = data_.nearestIdx( crd );
    if ( !data_.positions().validIdx(posidx) )
	return false;

    trcnr = data_.positions()[posidx].nr_;
    spnr = spnrs_.validIdx(posidx) ? spnrs_[posidx] : mUdf(int);
    return true;
}


const char* Survey::Geometry2D::getName() const
{
    return data_.lineName().buf();
}


Coord Survey::Geometry2D::toCoord( int, int trcnr ) const
{
    PosInfo::Line2DPos pos;
    return data_.getPos(trcnr,pos) ? pos.coord_ : Coord::udf();
}


Coord Survey::Geometry2D::toCoord( int tracenr ) const
{
    return toCoord( -1, tracenr );
}


TrcKey Survey::Geometry2D::nearestTrace( const Coord& crd, float* dist ) const
{
    PosInfo::Line2DPos pos;
    if ( !data_.getPos(crd,pos,dist) )
	return TrcKey::udf();

    return TrcKey( getID(), pos.nr_ );
}


void Survey::Geometry2D::touch()
{
    Threads::Locker locker( lock_ );
    mSetSampling;
    trcdist_ = mUdf(float);
    linelength_ = mUdf(float);
}


float Survey::Geometry2D::averageTrcDist() const
{
    Threads::Locker locker( lock_ );
    if ( !mIsUdf(trcdist_) )
	return trcdist_;

    float max;
    data_.compDistBetwTrcsStats( max, trcdist_ );
    return trcdist_;
}


void Survey::Geometry2D::setAverageTrcDist( float trcdist )
{
    Threads::Locker locker( lock_ );
    trcdist_ = trcdist;
}


float Survey::Geometry2D::lineLength() const
{
    Threads::Locker locker( lock_ );
    if ( !mIsUdf(linelength_) )
	return linelength_;

    linelength_ = 0;
    for ( int idx=1; idx<data_.positions().size(); idx++ )
	linelength_ += mCast(float,data_.positions()[idx].coord_.distTo(
			data_.positions()[idx-1].coord_));

    return linelength_;
}


void Survey::Geometry2D::setLineLength( float ll )
{
    Threads::Locker locker( lock_ );
    linelength_ = ll;
}


Survey::Geometry::RelationType Survey::Geometry2D::compare(
				const Geometry& geom, bool usezrg ) const
{
    mDynamicCastGet( const Survey::Geometry2D*, geom2d, &geom );
    if ( !geom2d )
	return UnRelated;

    const PosInfo::Line2DData& mydata = data();
    const PosInfo::Line2DData& otherdata = geom2d->data();
    if ( !mydata.coincidesWith(otherdata) )
	return UnRelated;

    const StepInterval<int> mytrcrg = mydata.trcNrRange();
    const StepInterval<int> othtrcrg = otherdata.trcNrRange();
    const StepInterval<float> myzrg = mydata.zRange();
    const StepInterval<float> othzrg = otherdata.zRange();
    if ( mytrcrg == othtrcrg && (!usezrg || myzrg.isEqual(othzrg,1e-3)) )
	return Identical;
    if ( mytrcrg.includes(othtrcrg) && (!usezrg || myzrg.includes(othzrg)) )
	return SuperSet;
    if ( othtrcrg.includes(mytrcrg) && (!usezrg || othzrg.includes(myzrg)) )
	return SubSet;

    return Related;
}


StepInterval<float> Survey::Geometry2D::zRange() const
{
    return data_.zRange();
}


bool Survey::Geometry2D::includes( int line, int tracenr ) const
{
    return data_.indexOf(tracenr) >= 0;
}


BufferString Survey::Geometry2D::makeUniqueLineName( const char* lsnm,
						     const char* lnm )
{
    BufferString newlnm( lsnm );
    newlnm.add( "-" );
    newlnm.add( lnm );
    return newlnm;
}
