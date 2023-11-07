/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackmutedef.h"

#include "genericnumer.h"
#include "keystrs.h"
#include "mathfunc.h"
#include "seispsioprov.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "unitofmeasure.h"

namespace PreStack
{

const char* MuteDef::sKeyRefHor()	{ return "Reference Horizon";  }

MuteDef::MuteDef( const char* nm )
    : NamedObject(nm)
    , offsettype_(SI().xyInFeet() ? Seis::OffsetFeet : Seis::OffsetMeter)
    , zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{
}


MuteDef::MuteDef( const MuteDef& oth )
    : zdomaininfo_(nullptr)
{
    *this = oth;
}


MuteDef::~MuteDef()
{
    deepErase( fns_ );
    delete zdomaininfo_;
}


MuteDef& MuteDef::operator=( const MuteDef& oth )
{
    if ( &oth == this )
	return *this;

    setName( oth.name() );
    deepCopy( fns_, oth.fns_ );
    pos_ = oth.pos_;
    refhor_ = oth.refhor_;
    offsettype_ = oth.offsettype_;
    setZDomain( oth.zDomain() );
    ischanged_ = oth.ischanged_;

    return *this;
}


bool MuteDef::isOffsetInMeters() const
{
    return offsettype_ == Seis::OffsetMeter;
}


bool MuteDef::isOffsetInFeet() const
{
    return offsettype_ == Seis::OffsetFeet;
}


Seis::OffsetType MuteDef::offsetType() const
{
    return offsettype_;
}


const ZDomain::Info& MuteDef::zDomain() const
{
    return *zdomaininfo_;
}


bool MuteDef::zIsTime() const
{
    return zDomain().isTime();
}


bool MuteDef::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool MuteDef::zInFeet() const
{
    return zDomain().isDepthFeet();
}


MuteDef& MuteDef::setOffsetType( Seis::OffsetType typ )
{
    if ( Seis::isOffsetDist(typ) )
	offsettype_ = typ;

    return *this;
}


MuteDef& MuteDef::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );
    return *this;
}


void MuteDef::fillPar( IOPar& par ) const
{
    if ( refhor_.isUdf() )
	par.set( sKeyRefHor(), nullptr );
    else
	par.set( sKeyRefHor(), refhor_ );

    zDomain().fillPar( par );
    SeisPSIOProvider::setGatherOffsetType( offsettype_, par );
}


bool MuteDef::usePar( const IOPar& par )
{
    par.get( sKeyRefHor(), refhor_ );
    Seis::OffsetType offsettype;
    if ( SeisPSIOProvider::getGatherOffsetType(par,offsettype) &&
	 Seis::isOffsetDist(offsettype) )
	offsettype_ = offsettype;

    const ZDomain::Info* zinfo = ZDomain::get( par );
    if ( zinfo && (zinfo->isTime() || zinfo->isDepth()) )
	setZDomain( *zinfo );

    return true;
}


const UnitOfMeasure* MuteDef::zUnit() const
{
    return UnitOfMeasure::zUnit( zDomain() );
}


const UnitOfMeasure* MuteDef::offsetUnit() const
{
    return SeisPSIOProvider::offsetUnit( offsetType() );
}


int MuteDef::size() const
{ return fns_.size(); }


int MuteDef::indexOf( const BinID& bid ) const
{ return pos_.indexOf( bid ); }


PointBasedMathFunction& MuteDef::getFn( int idx )
{ return *fns_[idx]; }


BinID& MuteDef::getPos( int idx )
{ return pos_[idx]; }


const PointBasedMathFunction& MuteDef::getFn( int idx ) const
{ return *fns_[idx]; }


const BinID& MuteDef::getPos( int idx ) const
{ return pos_[idx]; }


void MuteDef::add( PointBasedMathFunction* fn , const BinID& pos )
{
    fns_ += fn;
    pos_ += pos;
    ischanged_ = true;
}


void MuteDef::remove( int idx )
{
    if ( !fns_.validIdx(idx) )
	return;

    delete fns_.removeSingle( idx );
    pos_.removeSingle( idx );
    ischanged_ = true;
}


float MuteDef::value( float offs, const BinID& pos ) const
{
    if ( fns_.isEmpty() || pos.isUdf() )
	return mUdf(float);

    if ( fns_.size() == 1 )
	return fns_[0]->getValue( offs );

    const Coord si00 = SI().transform(
	BinID(SI().inlRange(true).start,SI().crlRange(true).start) );
    const Coord si11 = SI().transform(
	BinID(SI().inlRange(true).stop,SI().crlRange(true).stop) );

    const double normalweight = si00.sqDistTo( si11 );

    const Coord centercrd( SI().transform(pos) );
    Stats::CalcSetup rcsetup( true ); //weighted
    Stats::RunCalc<float> calc( rcsetup.require(Stats::Average) );

    for ( int iloc=0; iloc<fns_.size(); iloc++ )
    {
	const Coord crd( SI().transform(pos_[iloc]) );
	const float val = fns_[iloc]->getValue( offs );
	const double sqdist = crd.sqDistTo( centercrd );
	if ( sqdist < 1 )
	    return val;

	calc.addValue( val, (float) (normalweight / sqdist) );
    }

    return (float) calc.average();
}


void MuteDef::computeIntervals( float offs, const BinID& pos,
			       TypeSet<Interval<float> >& res) const
{
    if ( fns_.isEmpty() )
	return;

    const Coord si00 = SI().transform(
	    BinID(SI().inlRange(true).start,SI().crlRange(true).start) );
    const Coord si11 = SI().transform(
	    BinID(SI().inlRange(true).stop,SI().crlRange(true).stop) );

    const double normalweight = si00.sqDistTo( si11 );
    const Coord centercrd( SI().transform(pos) );

    PointBasedMathFunction weightedfn;
    TypeSet<float> zvals; getAllZVals( zvals );
    for ( int idz=0; idz<zvals.size(); idz++ )
    {
	const float zval = zvals[idz];
	Stats::CalcSetup rcsetup( true ); //weighted
	Stats::RunCalc<float> calc( rcsetup.require(Stats::Average) );
	for ( int iloc=0; iloc<fns_.size(); iloc++ )
	{
	    if ( fns_[iloc]->isEmpty() )
		continue;

	    const Coord crd( SI().transform(pos_[iloc]) );
	    const double sqdist = crd.sqDistTo( centercrd );
	    const float offset = fns_[iloc]->getValue( zval );
	    calc.addValue( offset, (float) (normalweight / sqdist) );
	}
	weightedfn.add( zval, (float) calc.average() );
    }

    TypeSet<float> mutezvals;
    for ( int idz=0; idz<zvals.size()-1; idz ++ )
    {
	const float z0 = zvals[idz];
	const float z1 = zvals[idz+1];

	float z = z0;
	if ( findValue( weightedfn, z0, z1, z, offs, 1e-3 ) )
	    mutezvals += z;
    }

    for ( int imute=0; imute<mutezvals.size(); imute+=2 )
    {
	Interval<float> itv( mUdf(float), mUdf(float) );
	itv.start = mutezvals[imute];
	if ( mutezvals.validIdx(imute+1) )
	    itv.stop = mutezvals[imute+1];
	res += itv;
    }
}


void MuteDef::getAllZVals( TypeSet<float>& zvals ) const
{
    for ( int ifn=0; ifn<fns_.size(); ifn++ )
    {
	for ( int idx=0; idx<fns_[ifn]->size(); idx ++ )
	    zvals += fns_[ifn]->xVals()[idx];
    }

    sort_array( zvals.arr(), zvals.size() );
}


void MuteDef::setReferenceHorizon( const MultiID& mid )
{
    refhor_ = mid;
}


const MultiID& MuteDef::getReferenceHorizon() const
{
    return refhor_;
}

} // namespace PreStack
