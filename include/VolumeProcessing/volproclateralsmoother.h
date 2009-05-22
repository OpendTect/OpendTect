#ifndef vollateralprocsmoother_h
#define vollateralprocsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: volproclateralsmoother.h,v 1.1 2009-05-22 18:41:55 cvskris Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"
#include "array2dfilter.h"

template <class T> class Smoother3D;

namespace VolProc
{
    
mClass LateralSmoother : public Step
{
public:
    static void			initClass();
    
				~LateralSmoother();
				LateralSmoother(Chain&);

    const char*			type() const;
    bool			needsInput(const HorSampling&) const;
    HorSampling			getInputHRg(const HorSampling&) const;

    void			setPars(const Array2DFilterPars&);
    const Array2DFilterPars&	getPars() const	{ return pars_; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
    static const char*		sKeyType()	{ return "LateralSmoother"; }
    static const char*		sUserName()	{ return "Lateral Smoother"; }

    Task*			createTask();

protected:
    static const char*		sKeyIsMedian()	{ return "Is Median"; }
    static const char*		sKeyIsWeighted(){ return "Is Weighted"; }
    static Step*		create(Chain&);
    Array2DFilterPars		pars_;
};

}; //namespace


#endif
