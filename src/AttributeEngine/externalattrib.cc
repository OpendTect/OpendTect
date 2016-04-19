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


RefMan<RegularSeisDataPack> ExtAttribCalc::createAttrib( const TrcKeyZSampling&, DataPack::ID,
					  TaskRunner* )
{ return 0; }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


RefMan<RegularSeisDataPack> ExtAttribCalc::createAttrib( const TrcKeyZSampling&,const LineKey&,
					  TaskRunner* )
{ return 0; }


} // namespace Attrib
