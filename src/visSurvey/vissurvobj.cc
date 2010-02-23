/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.51 2010-02-23 20:59:37 cvskris Exp $";

#include "vissurvobj.h"

#include "attribsel.h"
#include "iopar.h"
#include "seisbuf.h"

namespace visSurvey {

    float SurveyObject::sDefMaxDist()	{ return 10; }

SurveyObject::AttribFormat SurveyObject::getAttributeFormat( int attrib ) const
{ return SurveyObject::None; }

int SurveyObject::nrAttribs() const
{ return getAttributeFormat()==SurveyObject::None ? 0 : 1; }

bool SurveyObject::canAddAttrib(int) const
{ return canHaveMultipleAttribs(); }

bool SurveyObject::canRemoveAttrib() const
{ return canHaveMultipleAttribs() && nrAttribs()>1; }


void SurveyObject::setColTabMapperSetup( int, const ColTab::MapperSetup&,
					 TaskRunner*)
{}


bool SurveyObject::canHandleColTabSeqTrans(int) const
{ return true; }


const ColTab::MapperSetup* SurveyObject::getColTabMapperSetup( int ) const
{ return 0; }


const ColTab::MapperSetup* SurveyObject::getColTabMapperSetup( int attrib,
							    int version ) const
{ return getColTabMapperSetup( attrib ); }


void SurveyObject::setColTabSequence( int, const ColTab::Sequence&, TaskRunner*)
{}


void SurveyObject::setTraceData( int, SeisTrcBuf& trcs, TaskRunner* )
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
    const char* zdomain = as->zDomainKey();
    return zdomain && *zdomain;
}


void SurveyObject::fillSOPar( IOPar& par, TypeSet<int>& saveids ) const
{ par.setYN( sKeyLocked(), locked_ ); }


int SurveyObject::useSOPar( const IOPar& par )
{
    locked_ = false;
    par.getYN( sKeyLocked(), locked_ );
    return 1;
}

}; // namespace visSurvey

