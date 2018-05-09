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


/*!\brief a Step that smooths the input data. */

mExpClass(VolumeProcessing) Smoother : public Step
{ mODTextTranslationClass(Smoother)
public:
			mDefaultFactoryInstantiation(
				Step, Smoother,
				"Smoother", tr("Smoother"))

			Smoother();
			~Smoother();
    virtual void	releaseData();

    bool		canHandle2D() const	{ return true; }
    bool		setOperator(const char*,float param,
				    int inlsz,int crlsz,int zsz);
			//!<Size is set in multiples of inl/crl/z-step from SI.
    int			inlSz() const;
    int			crlSz() const;
    int			zSz() const;
    const char*		getOperatorName() const;
    float		getOperatorParam() const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

private:

    virtual ReportingTask*	createTask();

    virtual bool	needsFullVolume() const		{ return true; }
    virtual bool	canInputAndOutputBeSame() const	{ return false; }
    virtual bool	areSamplesIndependent() const	{ return true; }

    virtual BinID	getHorizontalStepout() const;
    virtual int		getVerticalStepout() const;
    virtual od_int64	extraMemoryUsage(OutputSlotID,
					 const TrcKeyZSampling&) const
			{ return 0; }

    Smoother3D<float>*	smoother_;

    static const char*	sKeyZStepout()		{ return "ZStepout"; }

};

} // namespace VolProc
