/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2003
 RCS:           $Id: viscoltabmod.cc,v 1.6 2005-02-04 14:31:34 kristofer Exp $
________________________________________________________________________

-*/



#include "viscoltabmod.h"

#include "dataclipper.h"
#include "visdataman.h"
#include "scaler.h"
#include "iopar.h"

namespace visBase
{

mCreateFactoryEntry( VisColTabMod );

const char* VisColTabMod::clipratestr	= "Cliprate";
const char* VisColTabMod::rangestr 	= "Range";
const char* VisColTabMod::reversestr 	= "Reverse display";
const char* VisColTabMod::useclipstr 	= "Use clipping";


VisColTabMod::VisColTabMod()
    : range(Interval<float>(0,0))
    , cliprate0(0.025)
    , cliprate1(0.025)
    , useclip(true)
    , reverse(false)
    , datascale( *new LinScaler(0,1) )
{
}


VisColTabMod::~VisColTabMod()
{
    delete &datascale;
}


float VisColTabMod::clipRate( bool cr0 ) const
{
    return cr0 ? cliprate0 : cliprate1;
}


void VisColTabMod::setClipRate( float cr0, float cr1 )
{
    if ( mIsEqual(cr0,cliprate0,mDefEps)
      && mIsEqual(cr1,cliprate1,mDefEps) ) return;

    cliprate0 = cr0;
    cliprate1 = cr1;
}


void VisColTabMod::setRange( const Interval<float>& rg )
{
    range.start = rg.start;
    range.stop = rg.stop;
}


void VisColTabMod::setScale( const float* values, int nrvalues )
{
    if ( !values ) return;

    Interval<float> intv(0,0);
    if ( useclip )
    {
	DataClipper clipper( cliprate0, cliprate1 );
	clipper.putData( values, nrvalues );
	clipper.calculateRange();
	range = clipper.getRange();
    }

    datascale.factor = 1.0/range.width();
    datascale.constant = -range.start * datascale.factor;
}


const LinScaler& VisColTabMod::getScale() const
{
    return datascale;
}


int VisColTabMod::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    par.get( clipratestr, cliprate0, cliprate1 );
    par.get( rangestr, range.stop, range.start );
    par.getYN( reversestr, reverse );
    par.getYN( useclipstr, useclip );

    return 1;
}


void VisColTabMod::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( clipratestr, cliprate0, cliprate1 );
    par.set( rangestr, range.start, range.stop );
    par.setYN( reversestr, reverse );
    par.setYN( useclipstr, useclip );
}

}; // namespace visBase
