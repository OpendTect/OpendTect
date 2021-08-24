/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2006
________________________________________________________________________

-*/

#include "externalattrib.h"

#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


DataPack::ID ExtAttribCalc::createAttrib( const TrcKeyZSampling&, DataPack::ID,
					  TaskRunner* )
{ return DataPack::cNoID(); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


DataPack::ID ExtAttribCalc::createAttrib( const TrcKeyZSampling&,const LineKey&,
					  TaskRunner* )
{ return DataPack::cNoID(); }


} // namespace Attrib
