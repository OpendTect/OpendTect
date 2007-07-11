/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.cc,v 1.3 2007-07-11 21:06:34 cvsyuancheng Exp $
________________________________________________________________________

-*/

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
    if ( fns_.size() < 1 )
	return 0;
    else if ( fns_.size() == 1 )
	return fns_[0]->getValue( offs );

    const Coord centercrd( SI().transform(pos) );
    Stats::RunCalcSetup rcsetup( true ); //weighted
    Stats::RunCalc<float> calc( rcsetup.require(Stats::Average) );

    for ( int iloc=0; iloc<fns_.size(); iloc++ )
    {
	const Coord crd( SI().transform(pos_[iloc]) );
	const float val = fns_[iloc]->getValue( offs );
	const double sqdist = crd.sqDistTo( centercrd );
	if ( sqdist < 1 ) return val;
	calc.addValue( val, 1 / sqdist );
    }

    return calc.mean();
}
