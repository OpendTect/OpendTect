/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.30 2005-11-11 22:36:08 cvskris Exp $";

#include "vissurvobj.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "iopar.h"
#include "linsolv.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vistransform.h"

namespace visSurvey {

float SurveyObject::sDefMaxDist = 10;

bool SurveyObject::setDataVolume( bool color, const Attrib::DataCubes* )
{ return true; }


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
