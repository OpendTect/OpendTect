/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2006
 RCS:           $Id: externalattrib.cc,v 1.4 2008-05-07 20:06:25 cvskris Exp $
________________________________________________________________________

-*/

#include "externalattrib.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplFactory1Param(  ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, DataPack::ID )
{ return DataPack::cNoID; }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>& )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf )
{ return false; }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling&, const LineKey& )
{ return DataPack::cNoID; }


bool ExtAttribCalc::isIndexes() const
{ return false; }


} // namespace Attrib
