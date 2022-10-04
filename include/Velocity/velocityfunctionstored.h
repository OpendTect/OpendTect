#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "velocityfunction.h"
#include "binidvalset.h"

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
    StepInterval<float>		getAvailableZ() const override;


protected:
    bool			computeVelocity(float z0, float dz, int nr,
						float* res) const override;

    bool			zit_;
    TypeSet<float>		zval_;
    TypeSet<float>		vel_;
};


mExpClass(Velocity) StoredFunctionSource : public FunctionSource
{ mODTextTranslationClass(StoredFunctionSource);
public:
				mDefaultFactoryInstanciationBase(
				    "StoredVelFunc", 
				    toUiString(sFactoryKeyword()));

				StoredFunctionSource();
    static IOObjContext&	ioContext();

    const VelocityDesc&		getDesc() const override { return desc_; }

    bool			zIsTime() const;
    bool			load(const MultiID&);
    bool			store(const MultiID&);

    StoredFunction*		createFunction(const BinID&) override;

    void			getAvailablePositions(
						BinIDValueSet&) const override;
    bool			getVel(const BinID&,TypeSet<float>& zvals,
				       TypeSet<float>& vel);

    void			setData(const BinIDValueSet&,
					const VelocityDesc&,bool zit);

    static const char*		sKeyZIsTime();
    static const char*		sKeyVelocityFunction();
    static const char*		sKeyVelocityType();

protected:
    void			fillIOObjPar(IOPar&) const;

    static FunctionSource*	create(const MultiID&);
				~StoredFunctionSource();

    BinIDValueSet		veldata_;
    bool			zit_;
    VelocityDesc		desc_;
};

} // namespace Vel
