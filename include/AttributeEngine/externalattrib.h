#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"

#include "factory.h"
#include "seisdatapack.h"
#include "sets.h"
#include "uistring.h"

class BinIDValueSet;
class LineKey;
class SeisTrcBuf;
class TaskRunner;
class TrcKeyZSampling;


namespace Attrib
{

class SelSpec;

/*!
\brief Generic class for attribs that does not come from the attribute engine.
*/

mExpClass(AttributeEngine) ExtAttribCalc
{
public:
    virtual			~ExtAttribCalc();
				mOD_DisableCopy(ExtAttribCalc)

    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
				/*!<\returns if this object can
				     compute it or not. */

    virtual ConstRefMan<RegularSeisDataPack>
				createAttrib(const TrcKeyZSampling&,
					     const RegularSeisDataPack* prevdp,
					     TaskRunner*);
    virtual ConstRefMan<RegularSeisDataPack>
				createAttrib(const TrcKeyZSampling&,
					     TaskRunner*);
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&,
					     TaskRunner*);
    virtual bool		createAttrib(const BinIDValueSet&,SeisTrcBuf&,
					     TaskRunner*);
    virtual ConstRefMan<RandomSeisDataPack>
				createRdmTrcAttrib(const ZGate&,
						   const RandomLineID&,
						   TaskRunner*);

    virtual bool		isIndexes() const	{ return false; }

    uiString			errmsg_;

protected:
				ExtAttribCalc();
};


mDefineFactory1Param( AttributeEngine, ExtAttribCalc, const Attrib::SelSpec&,
		      ExtAttrFact );


} // namespace Attrib
