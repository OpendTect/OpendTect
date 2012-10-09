/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";



#include "emsurfaceedgelineimpl.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "iopar.h"
#include "mathfunc.h"
#include "ranges.h"
#include "sorting.h"
#include "survinfo.h"
#include "errh.h"

#include <math.h>


namespace EM 
{

mEdgeLineSegmentFactoryEntry( TerminationEdgeLineSegment );
mEdgeLineSegmentFactoryEntry( SurfaceConnectLine );

const char* SurfaceConnectLine::connectingsectionstr = "Connecting Segment";

bool SurfaceConnectLine::internalIdenticalSettings(
					const EdgeLineSegment& els ) const
{
    return
	reinterpret_cast<const SurfaceConnectLine*>(&els)->connectingsection ==
			connectingsection &&
			EdgeLineSegment::internalIdenticalSettings(els);
}


bool SurfaceConnectLine::isNodeOK(const RowCol& rc) const
{
    return EdgeLineSegment::isNodeOK(rc) &&
	   horizon_.isDefined(connectingsection,rc.toInt64());
}


void SurfaceConnectLine::fillPar(IOPar& par) const
{
    EdgeLineSegment::fillPar(par);
    par.set(connectingsectionstr,connectingsection);
}


bool SurfaceConnectLine::usePar(const IOPar& par)
{
    int dummy;
    const bool res = EdgeLineSegment::usePar(par) &&
		     par.get(connectingsectionstr,dummy);
    if ( res ) connectingsection = dummy;
    return res;
}


} //namespace
