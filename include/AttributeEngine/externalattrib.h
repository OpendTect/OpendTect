#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "factory.h"
#include "datapack.h"
#include "sets.h"
#include "uistring.h"

class BinIDValueSet;
class TrcKeyZSampling;
class LineKey;
class SeisTrcBuf;
class TaskRunner;


namespace Attrib
{

class SelSpec;

/*!
\brief Generic class for attribs that does not come from the attribute engine.
*/

mExpClass(AttributeEngine) ExtAttribCalc
{
public:
				ExtAttribCalc();
    virtual			~ExtAttribCalc();

    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
				/*!<\returns if this object can
				     compute it or not. */

    virtual DataPackID		createAttrib(const TrcKeyZSampling&,
					     DataPackID, TaskRunner*);
    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&,
					     TaskRunner*);
    virtual bool		createAttrib(const BinIDValueSet&, SeisTrcBuf&,
					     TaskRunner*);
    virtual DataPackID		createAttrib(const TrcKeyZSampling&,
					     const LineKey&,TaskRunner*);

    virtual bool		isIndexes() const	{ return false; }

    uiString			errmsg_;
};


mDefineFactory1Param( AttributeEngine, ExtAttribCalc, const Attrib::SelSpec&,
		      ExtAttrFact );


} // namespace Attrib
