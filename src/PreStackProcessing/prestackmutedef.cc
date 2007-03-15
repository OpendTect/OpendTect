/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.cc,v 1.1 2007-03-15 17:28:52 cvskris Exp $
________________________________________________________________________

-*/

#include "prestackmutedef.h"

#include "mathfunc.h"
#include "statruncalc.h"
#include "survinfo.h"

using namespace PreStack;


MuteDef::MuteDef( const char* nm )
    : NamedObject( nm )
{}


MuteDef::~MuteDef()
{ deepErase( fns_ ); }


int MuteDef::size() const
{ return fns_.size(); }


PointBasedMathFunction& MuteDef::getFn( int idx )
{ return *fns_[idx]; }


BinID& MuteDef::getPos( int idx )
{ return pos_[idx]; }


const PointBasedMathFunction& MuteDef::getFn( int idx ) const
{ return *fns_[idx]; }


const BinID& MuteDef::getPos( int idx ) const
{ return pos_[idx]; }


void MuteDef::add( PointBasedMathFunction* fn , const BinID& pos )
{ fns_ += fn; pos_ += pos; }


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
