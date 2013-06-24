#ifndef velocityfunctionstored_h
#define velocityfunctionstored_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Umesh Sinha
Date:          Sep 2008
RCS:           $Id$
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "velocityfunction.h"
#include "binidvalset.h"

class BinIDValueSet;
class MultiID;
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
    StepInterval<float> 	getAvailableZ() const;


protected:
    bool 			computeVelocity(float z0, float dz, int nr,
						float* res) const;

    bool			zit_;
    TypeSet<float>         	zval_;
    TypeSet<float>              vel_;
};


mExpClass(Velocity) StoredFunctionSource : public FunctionSource
{
public:
    mDefaultFactoryInstanciationBase( "StoredVelFunc", sFactoryKeyword());

    				StoredFunctionSource();
    static IOObjContext&	ioContext();

    const VelocityDesc&         getDesc() const         { return desc_; }

    bool                        zIsTime() const;
    bool                        load(const MultiID&);
    bool                        store(const MultiID&);

    StoredFunction*            	createFunction(const BinID&);
   
    void			getAvailablePositions(BinIDValueSet&) const;
    bool			getVel(const BinID&,TypeSet<float>& zvals,
	                               TypeSet<float>& vel);

    void			setData(const BinIDValueSet&,
	    				const VelocityDesc&,bool zit);

    static const char*          sKeyZIsTime();
    static const char*          sKeyVelocityFunction();
    static const char*          sKeyVelocityType();

protected:
    void			fillIOObjPar(IOPar&) const;

    static FunctionSource* 	create(const MultiID&);
				~StoredFunctionSource();
    
    BinIDValueSet		veldata_;
    bool			zit_;
    VelocityDesc		desc_; 
};

}; // namespace


#endif

