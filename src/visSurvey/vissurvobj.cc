/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.41 2007-12-18 12:19:35 cvsnanne Exp $";

#include "vissurvobj.h"

#include "attribsel.h"
#include "iopar.h"
#include "seisbuf.h"

namespace visSurvey {

float SurveyObject::sDefMaxDist = 10;

SurveyObject::AttribFormat SurveyObject::getAttributeFormat() const
{ return SurveyObject::None; }

int SurveyObject::nrAttribs() const
{ return getAttributeFormat()==SurveyObject::None ? 0 : 1; }

bool SurveyObject::canAddAttrib() const
{ return canHaveMultipleAttribs(); }

bool SurveyObject::canRemoveAttrib() const
{ return canHaveMultipleAttribs() && nrAttribs()>1; }

void SurveyObject::setTraceData( int, SeisTrcBuf& trcs )
{ trcs.deepErase(); }


BufferString SurveyObject::getResolutionName( int res ) const
{
    if ( res == 1 ) return "Moderate";
    if ( res == 2 ) return "High";
    else return "Default";
}


bool SurveyObject::alreadyTransformed( int attrib ) const
{
    const Attrib::SelSpec* as = getSelSpec( attrib );
    if ( !as ) return false;
    const char* depthdomain = as->depthDomainKey();
    return depthdomain && *depthdomain;
}


void SurveyObject::fillSOPar( IOPar& par ) const
{ par.setYN( sKeyLocked(), locked_ ); }


bool SurveyObject::useSOPar( const IOPar& par )
{
    locked_ = false;
    par.getYN( sKeyLocked(), locked_ );
    return true;
}

}; // namespace visSurvey

