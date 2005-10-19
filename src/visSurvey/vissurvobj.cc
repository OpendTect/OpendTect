/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.29 2005-10-19 22:03:40 cvskris Exp $";

#include "vissurvobj.h"

#include "arrayndimpl.h"
#include "attribslice.h"
#include "iopar.h"
#include "linsolv.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vistransform.h"

namespace visSurvey {

float SurveyObject::sDefMaxDist = 10;

bool SurveyObject::setDataVolume( bool color, Attrib::SliceSet* slc )
{
    slc->unRef();
    return true;
}


void SurveyObject::setTraceData( bool color, SeisTrcBuf& trcs )
{
    trcs.deepErase();
}


BufferString SurveyObject::getResolutionName( int res ) const
{
    if ( res == 1 ) return "Moderate";
    if ( res == 2 ) return "High";
    else return "Default";
}


void SurveyObject::setScene( Scene* scn )
{
    scene_ = scn;
    setUpConnections();
}


bool SurveyObject::setDataTransform( ZAxisTransform* )
{ return false; }


}; // namespace visSurvey
