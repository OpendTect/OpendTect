/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2006
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: externalattrib.cc,v 1.10 2012-05-02 11:52:58 cvskris Exp $";

#include "externalattrib.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, DataPack::ID,
					  TaskRunner* )
{ return DataPack::cNoID(); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, const LineKey&,
					  TaskRunner* )
{ return DataPack::cNoID(); }


} // namespace Attrib
