#ifndef externalattrib_h
#define externalattrib_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: externalattrib.h,v 1.9 2008-05-07 20:06:25 cvskris Exp $
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
    virtual			~ExtAttribCalc()			{}
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
    				/*!<\returns if this object can 
				     compute it or not. */
    virtual DataPack::ID	createAttrib(const CubeSampling&,
					     DataPack::ID=DataPack::cNoID);
    				//!<Utilizes createAttrib(CS,DC)
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&);
    virtual bool		createAttrib(const BinIDValueSet&, SeisTrcBuf&);
    virtual DataPack::ID	createAttrib(const CubeSampling&,
	    				     const LineKey&);

    virtual bool		isIndexes() const;

    BufferString		errmsg_;
};


mDefineFactory1Param( ExtAttribCalc, const Attrib::SelSpec&, ExtAttrFact );


} // namespace Attrib

#endif
