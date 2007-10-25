#ifndef externalattrib_h
#define externalattrib_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: externalattrib.h,v 1.6 2007-10-25 21:16:53 cvskris Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "datapack.h"
#include "sets.h"

class BinIDValueSet;
class CubeSampling;
class LineKey;
class SeisTrcBuf;


namespace Attrib
{

class SelSpec;
class DataCubes;

/*! Generic class for attribs that does not come from the attribute engine. */


class ExtAttribCalc
{
public:
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
    				/*!<\returns if this object can 
				     compute it or not. */
    virtual const DataCubes*	createAttrib(const CubeSampling&,
				             const DataCubes*)		= 0;
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&) = 0;
    virtual bool		createAttrib(const BinIDValueSet&,
						     SeisTrcBuf&)	= 0;
    virtual DataPack::ID	createAttrib(const CubeSampling&,
	    				     const LineKey&)		= 0;

    virtual bool		isIndexes() const			= 0;

    BufferString		errmsg_;
};

mDefineFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


} // namespace Attrib

#endif
