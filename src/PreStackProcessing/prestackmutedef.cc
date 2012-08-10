/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: prestackmutedef.cc,v 1.15 2012-08-10 04:11:24 cvssalil Exp $";

#include "prestackmutedef.h"

#include "genericnumer.h"
#include "mathfunc.h"
#include "statruncalc.h"
#include "survinfo.h"

using namespace PreStack;


MuteDef::MuteDef( const char* nm )
    : NamedObject( nm )
    , ischanged_( false )  
{}


MuteDef::MuteDef( const MuteDef& b )
{ (*this) = b; }


MuteDef::~MuteDef()
{ deepErase( fns_ ); }


MuteDef& MuteDef::operator=(const MuteDef& b)
{
    refhor_ = b.refhor_;
    setName( b.name() );
    deepCopy( fns_, b.fns_ );
    pos_ = b.pos_;
    ischanged_ = b.ischanged_;
    return *this;
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
    if ( idx<0 || idx>=size() )
	return;

    delete fns_.remove( idx );
    pos_.remove( idx );
    ischanged_ = true;
}


float MuteDef::value( float offs, const BinID& pos ) const
{
    if ( pos.inl<0 || pos.crl<0 )
	return mUdf(float);

    if ( fns_.size() < 1 )
	return mUdf(float);
    else if ( fns_.size() == 1 )
	return fns_[0]->getValue( offs );

    const Coord si00 = SI().transform(
	    BinID(SI().inlRange(true).start, SI().crlRange(true).start ) );
    const Coord si11 = SI().transform(
	    BinID(SI().inlRange(true).stop, SI().crlRange(true).stop ) );

    const double normalweight = si00.sqDistTo( si11 );

    const Coord centercrd( SI().transform(pos) );
    Stats::CalcSetup rcsetup( true ); //weighted
    Stats::RunCalc<float> calc( rcsetup.require(Stats::Average) );

    for ( int iloc=0; iloc<fns_.size(); iloc++ )
    {
	const Coord crd( SI().transform(pos_[iloc]) );
	const float val = fns_[iloc]->getValue( offs );
	const double sqdist = crd.sqDistTo( centercrd );
	if ( sqdist < 1 ) return val;
	calc.addValue( val, (float) (normalweight / sqdist) );
    }

    return (float) calc.average();
}


void MuteDef::computeIntervals( float offs, const BinID& pos,
			       TypeSet<Interval<float> >& res) const
{
    if ( pos.inl<0 || pos.crl<0 || fns_.isEmpty() )
	return;

    const Coord si00 = SI().transform(
	    BinID(SI().inlRange(true).start, SI().crlRange(true).start ) );
    const Coord si11 = SI().transform(
	    BinID(SI().inlRange(true).stop, SI().crlRange(true).stop ) );

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
{ refhor_ = mid; }


const MultiID& MuteDef::getReferenceHorizon() const
{ return refhor_; }
