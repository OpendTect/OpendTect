/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2003
 RCS:           $Id: viscoltabmod.cc,v 1.3 2003-11-07 12:22:02 bert Exp $
________________________________________________________________________

-*/



#include "viscoltabmod.h"

#include "dataclipper.h"
#include "visdataman.h"
#include "scaler.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::VisColTabMod );

const char* visBase::VisColTabMod::clipratestr	= "Cliprate";
const char* visBase::VisColTabMod::rangestr 	= "Range";
const char* visBase::VisColTabMod::reversestr 	= "Reverse display";
const char* visBase::VisColTabMod::useclipstr 	= "Use clipping";


visBase::VisColTabMod::VisColTabMod()
    : range(Interval<float>(0,0))
    , cliprate0(0.025)
    , cliprate1(0.025)
    , useclip(true)
    , reverse(false)
{
    for ( int idx=0; idx<5; idx++ )
	datascales += LinScaler( 0, 1 );
}


visBase::VisColTabMod::~VisColTabMod()
{
}


float visBase::VisColTabMod::clipRate( bool cr0 ) const
{
    return cr0 ? cliprate0 : cliprate1;
}


void visBase::VisColTabMod::setClipRate( float cr0, float cr1 )
{
    if ( mIS_ZERO(cr0-cliprate0) && mIS_ZERO(cr1-cliprate1) ) return;

    cliprate0 = cr0;
    cliprate1 = cr1;
}


void visBase::VisColTabMod::setRange( const Interval<float>& rg )
{
    range.start = rg.start;
    range.stop = rg.stop;
}


void visBase::VisColTabMod::setScale( const float* values, int nrvalues, 
				      int datatype )
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

    datascales[datatype].factor = 1.0/range.width();
    datascales[datatype].constant = -range.start * datascales[datatype].factor;
}


const LinScaler& visBase::VisColTabMod::getScale( int datatype ) const
{
    return datascales[datatype];
}


int visBase::VisColTabMod::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    par.get( clipratestr, cliprate0, cliprate1 );
    par.get( rangestr, range.stop, range.start );
    par.getYN( reversestr, reverse );
    par.getYN( useclipstr, useclip );

    return 1;
}


void visBase::VisColTabMod::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( clipratestr, cliprate0, cliprate1 );
    par.set( rangestr, range.start, range.stop );
    par.setYN( reversestr, reverse );
    par.setYN( useclipstr, useclip );
}
