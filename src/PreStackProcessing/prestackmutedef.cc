/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: prestackmutedef.cc,v 1.9 2011/10/26 14:20:13 cvsbruno Exp $";

#include "prestackmutedef.h"

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
	calc.addValue( val, normalweight / sqdist );
    }

    return calc.average();
}


void MuteDef::setReferenceHorizon( const MultiID& mid )
{ refhor_ = mid; }


const MultiID& MuteDef::getReferenceHorizon() const
{ return refhor_; }
