#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Umesh Sinha
Date:          Sep 2008
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "velocityfunction.h"
#include "binnedvalueset.h"

class BinnedValueSet;
class IOObjContext;

namespace Vel
{

class StoredFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Picks. */

mExpClass(Velocity) StoredFunction : public Function
{
public:
				StoredFunction(StoredFunctionSource&);
    bool			moveTo(const BinID&);
    StepInterval<float>	getAvailableZ() const;


protected:
    bool			computeVelocity(float z0, float dz, int nr,
						float* res) const;

    bool			zit_;
    TypeSet<float>	zval_;
    TypeSet<float>              vel_;
};


mExpClass(Velocity) StoredFunctionSource : public FunctionSource
{ mODTextTranslationClass(StoredFunctionSource);
public:
				mDefaultFactoryInstantiationBase(
				    "StoredVelFunc",
				    toUiString(sFactoryKeyword()));

				StoredFunctionSource();
    static IOObjContext&	ioContext();

    const VelocityDesc&         getDesc() const         { return desc_; }

    bool                        zIsTime() const;
    bool                        load(const DBKey&);
    bool                        store(const DBKey&);

    StoredFunction*	createFunction(const BinID&);

    void			getAvailablePositions(BinnedValueSet&) const;
    bool			getVel(const BinID&,TypeSet<float>& zvals,
	                               TypeSet<float>& vel);

    void			setData(const BinnedValueSet&,
					const VelocityDesc&,bool zit);

    static const char*          sKeyZIsTime();
    static const char*          sKeyVelocityFunction();
    static const char*          sKeyVelocityType();

protected:
    void			fillIOObjPar(IOPar&) const;

    static FunctionSource*	create(const DBKey&);
				~StoredFunctionSource();

    BinnedValueSet		veldata_;
    bool                        zit_;
    VelocityDesc                desc_;
};

} // namespace Vel
