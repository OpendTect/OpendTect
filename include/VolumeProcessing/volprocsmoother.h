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

template <class T> class Smoother3D;

namespace VolProc
{

/*!
\brief A subclass of Step to smoothen volumes.
*/

mExpClass(VolumeProcessing) Smoother : public Step
{ mODTextTranslationClass(Smoother)
public:
			mDefaultFactoryInstantiation(
				VolProc::Step, Smoother,
				"Smoother", tr("Smoother"))

			~Smoother();
			Smoother();

    bool		needsInput() const override		{ return true; }
    TrcKeySampling	getInputHRg(const TrcKeySampling&) const override;
    StepInterval<int>	getInputZRg(const StepInterval<int>&) const override;
    StepInterval<int>	getInputZRgWithGeom(const StepInterval<int>&,
				    Pos::GeomID) const override;

    bool		setOperator(const char*,float param,
				    int inlsz,int crlsz,int zsz);
			//!<Size is set in multiples of inl/crl/z-step from SI.
    int			inlSz() const;
    int			crlSz() const;
    int			zSz() const;
    const char*		getOperatorName() const;
    float		getOperatorParam() const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		releaseData() override;
    bool		canInputAndOutputBeSame() const override
			{ return false; }

    bool		needsFullVolume() const override	{ return true; }
    bool		canHandle2D() const override		{ return true; }

    Task*		createTask() override;

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    static const char*	sKeyZStepout()		{ return "ZStepout"; }

    bool		prepareComp(int) override		{ return true; }

    Smoother3D<float>*	smoother_;

private:

    void		setStepouts();

};

} // namespace VolProc

