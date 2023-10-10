#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "binidvalset.h"
#include "velocityfunction.h"

class BinIDValueSet;
class IOObjContext;

namespace Vel
{

class StoredFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Picks. */

mExpClass(Velocity) StoredFunction : public Function
{
public:
				StoredFunction(StoredFunctionSource&);
				~StoredFunction();

    bool			moveTo(const BinID&) override;
    ZSampling			getAvailableZ() const override;

protected:
    bool			computeVelocity(float z0,float dz,int sz,
						float* res) const override;

    TypeSet<double>		zval_;
    TypeSet<double>		vel_;
};


mExpClass(Velocity) StoredFunctionSource : public FunctionSource
{ mODTextTranslationClass(StoredFunctionSource);
public:
				mDefaultFactoryInstanciationBase(
				    "StoredVelFunc",
				    ::toUiString(sFactoryKeyword()));

				StoredFunctionSource();
    static IOObjContext&	ioContext();

    const VelocityDesc&		getDesc() const override { return desc_; }

    bool			setFrom(const MultiID&);
    bool			store(const MultiID&);

    StoredFunction*		createFunction(const BinID&) override;

    void			getAvailablePositions(
						BinIDValueSet&) const override;
    bool			getVel(const BinID&,TypeSet<double>& zvals,
				       TypeSet<double>& vel);

    void			setData(const BinIDValueSet&,
					const VelocityDesc&,
					const ZDomain::Info&);

    static const char*		sKeyVelocityType();

protected:
				~StoredFunctionSource();

    void			fillIOObjPar(IOPar&) const;

    static FunctionSource*	create(const MultiID&);

    BinIDValueSet		veldata_;
    VelocityDesc&		desc_;
};

} // namespace Vel
