#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "array2dfilter.h"


namespace VolProc
{

/*!
\brief Lateral smoother. Subclass of Step.
*/

mExpClass(VolumeProcessing) LateralSmoother : public Step
{ mODTextTranslationClass(LateralSmoother);
public:
			mDefaultFactoryInstantiation(
				VolProc::Step, LateralSmoother,
				"LateralSmoother", tr("Lateral Smoother") )

			LateralSmoother();
			~LateralSmoother();

    bool		needsInput() const override;
    TrcKeySampling	getInputHRg(const TrcKeySampling&) const override;

    void		setPars(const Array2DFilterPars&);
    void		setMirrorEdges(bool yn) { mirroredges_=yn; }
    void		setFixedValue(float v) { fixedvalue_=v; }
    void		setInterpolateUdfs(bool b) {interpolateundefs_=b;}

    const Array2DFilterPars& getPars() const	{ return pars_; }
    bool		getMirrorEdges() const	{ return mirroredges_; }
    float		getFixedValue() const	{ return fixedvalue_; }
    bool		getInterpolateUdfs() const {return interpolateundefs_;}

    void	fillPar(IOPar&) const override;
    bool	usePar(const IOPar&) override;

    bool	areSamplesIndependent() const override	{ return true; }

    bool		canInputAndOutputBeSame() const override {return true;}
    bool		needsFullVolume() const override	{return false;}
    bool		canHandle2D() const override	{ return false; }

    Task*		createTask() override;

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    static const char*	sKeyIsMedian()		{ return "Is Median"; }
    static const char*	sKeyIsWeighted()	{ return "Is Weighted"; }
    static const char*	sKeyMirrorEdges()	{ return "Mirror Edges"; }
    static const char*	sKeyInterpolateUdf()	{ return "Interpolate Udf";}
    static const char*	sKeyFixedValue()	{ return "Fixed Value"; }

    Array2DFilterPars	pars_;

    bool		mirroredges_;
    bool		interpolateundefs_;
    float		fixedvalue_;

private:

    void		setStepouts();
};

} // namespace VolProc

