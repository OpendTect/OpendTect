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
				Step, LateralSmoother,
				"LateralSmoother", tr("Lateral Smoother") )

			LateralSmoother();
			~LateralSmoother();

    void		setPars(const Array2DFilterPars&);
    void		setMirrorEdges(bool yn) { mirroredges_=yn; }
    void		setFixedValue(float v) { fixedvalue_=v; }
    void		setInterpolateUdfs(bool b) {interpolateundefs_=b;}

    const Array2DFilterPars& getPars() const	{ return pars_; }
    bool		getMirrorEdges() const	{ return mirroredges_; }
    float		getFixedValue() const	{ return fixedvalue_; }
    bool		getInterpolateUdfs() const {return interpolateundefs_;}

private:

    virtual ReportingTask*	createTask();
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    virtual bool	needsFullVolume() const		{ return false; }
    virtual bool	canInputAndOutputBeSame() const	{ return true; }
    virtual bool	areSamplesIndependent() const	{ return true; }
    virtual bool	canHandle2D() const		{ return false; }
    virtual bool	needsInput() const;

    virtual BinID	getHorizontalStepout() const;
    virtual od_int64	extraMemoryUsage(OutputSlotID,
					 const TrcKeyZSampling&) const;

    Array2DFilterPars	pars_;
    bool		mirroredges_;
    bool		interpolateundefs_;
    float		fixedvalue_;

    static const char*	sKeyIsMedian()		{ return "Is Median"; }
    static const char*	sKeyIsWeighted()	{ return "Is Weighted"; }
    static const char*	sKeyMirrorEdges()	{ return "Mirror Edges"; }
    static const char*	sKeyInterpolateUdf()	{ return "Interpolate Udf";}
    static const char*	sKeyFixedValue()	{ return "Fixed Value"; }

};

} // namespace VolProc
