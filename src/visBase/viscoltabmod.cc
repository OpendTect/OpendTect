/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "viscoltabmod.h"

#include "coltab.h"
#include "dataclipper.h"
#include "iopar.h"
#include "scaler.h"
#include "visdataman.h"

mCreateFactoryEntry( visBase::VisColTabMod );

namespace visBase
{

const char* VisColTabMod::clipratestr()  { return  "Cliprate"; }
const char* VisColTabMod::rangestr()	 { return  "Range"; }
const char* VisColTabMod::reversestr()	 { return  "Reverse display"; }
const char* VisColTabMod::useclipstr()	 { return  "Use clipping"; }


VisColTabMod::VisColTabMod()
    : range(Interval<float>(0,0))
    , cliprate0(ColTab::defClipRate().start)
    , cliprate1(ColTab::defClipRate().stop)
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

    if ( useclip )
    {
	DataClipper clipper;
	clipper.setApproxNrValues( nrvalues, 5000 );
	clipper.putData( values, nrvalues );
	clipper.calculateRange(cliprate0, cliprate1, range );
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

    par.get( clipratestr(), cliprate0, cliprate1 );
    par.get( rangestr(), range );
    par.getYN( reversestr(), reverse );
    par.getYN( useclipstr(), useclip );

    return 1;
}


void VisColTabMod::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( clipratestr(), cliprate0, cliprate1 );
    par.set( rangestr(), range );
    par.setYN( reversestr(), reverse );
    par.setYN( useclipstr(), useclip );
}

}; // namespace visBase
