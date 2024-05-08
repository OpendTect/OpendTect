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


DataPackID ExtAttribCalc::createAttrib( const TrcKeyZSampling&, DataPackID,
					  TaskRunner* )
{ return DataPack::cNoID(); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


DataPackID ExtAttribCalc::createAttrib( const TrcKeyZSampling&, TaskRunner* )
{ return DataPack::cNoID(); }


} // namespace Attrib
