/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.1 2005-02-04 09:29:00 kristofer Exp $";

#include "attriboutput.h"


namespace Attrib
{

SliceSetOutput::SliceSetOutput( const CubeSampling& cs )
    : desiredvol( cs )
    , sliceset( 0 )
{
    const float dz = SI().zRange().step;
    sampleinterval.start = (int)cs.zrg.start/dz;
    const float stop = cs.zrg.start/dz;
    sampleinterval.stop = (int)stop;
    if ( stop-sampleinterval.stop )
	sampleinterval.stop++;
}


SliceSetOutput::~SliceSetOutput() { if ( sliceset ) sliceset->unRef(); }


bool SliceSetOutput::getDesiredVolume(CubeSampling& cs) const
{ cs=desiredvolume; return true; }


bool SliceSetOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


Interval<int> SliceSetOutput::getLocalZRange(const BinID&) const
{ return sampleinterval; }


void SliceSetOutput::collectData(const BinID& bid, const DataHolder& data )
{
    //TODO: Implement
}


AttribSliceSet* SliceSetOutput::getSliceSet() const
{
}


}; //namespace
