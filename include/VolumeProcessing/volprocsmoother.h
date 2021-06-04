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

    bool		needsInput() const { return true; }
    TrcKeySampling	getInputHRg(const TrcKeySampling&) const;
    StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
    StepInterval<int>	getInputZRgWithGeom(const StepInterval<int>&,
				    Survey::Geometry::ID) const;

    bool		setOperator(const char*,float param,
				    int inlsz,int crlsz,int zsz);
			//!<Size is set in multiples of inl/crl/z-step from SI.
    int			inlSz() const;
    int			crlSz() const;
    int			zSz() const;
    const char*		getOperatorName() const;
    float		getOperatorParam() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    void		releaseData();
    bool		canInputAndOutputBeSame() const	{ return false; }
    bool		needsFullVolume() const { return true; }
    bool		canHandle2D() const		{ return true; }

    Task*		createTask();

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

protected:

    static const char*	sKeyZStepout()		{ return "ZStepout"; }

    bool		prepareComp(int)	{ return true; }

    Smoother3D<float>*	smoother_;

private:

    void		setStepouts();

};

} // namespace VolProc

