#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2004
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "factory.h"
#include "datapack.h"
#include "sets.h"
#include "uistring.h"

class BinnedValueSet;
class TrcKeyZSampling;
class SeisTrcBuf;
class TaskRunner;
class RegularSeisDataPack;


namespace Attrib
{

class SelSpec;

/*!\brief Generic class for attribs that do not come from the attribute engine.

 TODO: add sme info so we know what it is and what it does ...

*/

mExpClass(AttributeEngine) ExtAttribCalc
{
public:

    mDefineFactory1ParamInClass( ExtAttribCalc, const Attrib::SelSpec&,
				 factory );

    virtual			~ExtAttribCalc()			{}
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
				/*!<\returns if this object can
				     compute it or not. */

    virtual RefMan<RegularSeisDataPack>	createAttrib(const TrcKeyZSampling&,
					     DataPack::ID, TaskRunner*);

    virtual bool		createAttrib(ObjectSet<BinnedValueSet>&,
					     TaskRunner*);
    virtual bool		createAttrib(const BinnedValueSet&, SeisTrcBuf&,
					     TaskRunner*);

    virtual bool		isIndexes() const	{ return false; }

    uiString			errmsg_;

};


} // namespace Attrib
