/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "externalattrib.h"

#include "attribsel.h"


namespace Attrib
{

mImplFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


ExtAttribCalc::ExtAttribCalc()
{}


ExtAttribCalc::~ExtAttribCalc()
{}


ConstRefMan<RegularSeisDataPack>
ExtAttribCalc::createAttrib( const TrcKeyZSampling&, const RegularSeisDataPack*,
			     TaskRunner* )
{ return nullptr; }


ConstRefMan<RegularSeisDataPack>
ExtAttribCalc::createAttrib( const TrcKeyZSampling&, TaskRunner* )
{ return nullptr; }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


ConstRefMan<RandomSeisDataPack>
ExtAttribCalc::createRdmTrcAttrib( const Interval<float>&,
				   RandomLineID, TaskRunner* )
{
    return nullptr;
}

} // namespace Attrib
