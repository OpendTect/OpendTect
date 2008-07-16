/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2006
 RCS:           $Id: externalattrib.cc,v 1.5 2008-07-16 16:59:41 cvskris Exp $
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


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling& cs,
	DataPack::ID dp, TaskRunner*	)
{ return createAttrib( cs, dp ); }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>& o,
       TaskRunner*	)
{ return createAttrib( o ); }


bool ExtAttribCalc::createAttrib( const BinIDValueSet& bv, SeisTrcBuf& buf,
       TaskRunner*	)
{ return createAttrib( bv, buf ); }


DataPack::ID ExtAttribCalc::createAttrib( const CubeSampling& cs,
	const LineKey& lk, TaskRunner* )
{ return createAttrib( cs, lk ); }


bool ExtAttribCalc::isIndexes() const
{ return false; }


} // namespace Attrib
