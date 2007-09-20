/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vissurvobj.cc,v 1.40 2007-09-20 06:05:23 cvssulochana Exp $";

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

SurveyObject::AttribFormat SurveyObject::getAttributeFormat() const
{ return SurveyObject::None; }


Coord3 SurveyObject::getNormal( const Coord3& pos ) const
{ return Coord3::udf(); }


bool SurveyObject::canHaveMultipleAttribs() const { return false; }


int SurveyObject::nrAttribs() const
{ return getAttributeFormat()==SurveyObject::None ? 0 : 1; }


bool SurveyObject::canAddAttrib() const
{ return canHaveMultipleAttribs(); }


bool SurveyObject::addAttrib()
{ return false; }


bool SurveyObject::canRemoveAttrib() const
{ return canHaveMultipleAttribs() && nrAttribs()>1; }


bool SurveyObject::removeAttrib( int )
{ return false; }


bool SurveyObject::swapAttribs( int, int )
{ return false; }


void SurveyObject::setAttribTransparency( int, unsigned char )
{}


unsigned char SurveyObject::getAttribTransparency( int ) const
{ return 0; }


const Attrib::SelSpec* SurveyObject::getSelSpec( int attrib ) const
{ return 0; }


const TypeSet<float>* SurveyObject::getHistogram( int attrib ) const
{ return 0; }


int SurveyObject::getColTabID( int attrib ) const 
{ return -1; }


bool SurveyObject::isClassification(int) const
{ return false; }


void SurveyObject::setClassification( int, bool )
{}


bool SurveyObject::isAngle(int) const
{ return false; }


void SurveyObject::setAngleFlag( int, bool )
{}


bool SurveyObject::isAttribEnabled( int ) const
{ return true; }


void SurveyObject::enableAttrib( int, bool )
{}


bool SurveyObject::setDataVolume( int, const Attrib::DataCubes* )
{ return true; }


const ZAxisTransform* SurveyObject::getDataTransform() const
{ return 0; }


const Attrib::DataCubes* SurveyObject::getCacheVolume( int attrib ) const
{ return 0; }


void SurveyObject::setTraceData( int, SeisTrcBuf& trcs )
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
}


bool SurveyObject::setDataTransform( ZAxisTransform* )
{ return false; }


void SurveyObject::fillSOPar( IOPar& par ) const
{
    par.setYN( sKeyLocked(), locked_ );
}


bool SurveyObject::useSOPar( const IOPar& par )
{
    locked_ = false;
    par.getYN( sKeyLocked(), locked_ );
    return true;
}

}; // namespace visSurvey

