#ifndef volproclateralsmoother_h
#define volproclateralsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: volproclateralsmoother.h,v 1.5 2010-10-04 19:56:14 cvskris Exp $
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
    static void		initClass();
    
			~LateralSmoother();
			LateralSmoother(Chain&);

    const char*		type() const;
    bool		needsInput(const HorSampling&) const;
    HorSampling		getInputHRg(const HorSampling&) const;

    void		setPars(const Array2DFilterPars&);
    void		setMirrorEdges(bool yn) { mirroredges_=yn; }
    void		setFixedValue(float v) { fixedvalue_=v; }
    void		setInterpolateUdfs(bool b) {interpolateundefs_=b;}

    const Array2DFilterPars&	getPars() const	{ return pars_; }
    bool		getMirrorEdges() const { return mirroredges_; }
    float		getFixedValue() const { return fixedvalue_; }
    bool		getInterpolateUdfs() const {return interpolateundefs_;}

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    
    bool		canInputAndOutputBeSame() const {return true;}
    bool		needsFullVolume() const		{return false;}

    static const char*	sKeyType()	{ return "LateralSmoother"; }
    static const char*	sUserName()	{ return "Lateral Smoother"; }

    Task*		createTask();

protected:
    static const char*	sKeyIsMedian()	{ return "Is Median"; }
    static const char*	sKeyIsWeighted(){ return "Is Weighted"; }
    static const char*	sKeyMirrorEdges(){ return "Mirror Edges"; }
    static const char*	sKeyInterpolateUdf(){ return "Interpolate Udf";}
    static const char*	sKeyFixedValue() { return "Fixed Value"; }
    
    static Step*	create(Chain&);
    Array2DFilterPars	pars_;

    bool		mirroredges_;
    bool		interpolateundefs_;
    float		fixedvalue_;
};

}; //namespace


#endif
