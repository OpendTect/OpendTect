#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"

class BinID;
namespace EM { class EMObject; class Horizon; }

namespace VolProc
{

/*!\brief Fills a volume with values.

  The top and bottom of the volume are either the survey top/bottom,
  or horizons. The values are fixed at the top boundary (either horizon or
  survey top) and change either with a fixed gradient or to a fixed value at
  the bottom boundary.
*/

mExpClass(VolumeProcessing) HorInterFiller : public Step
{ mODTextTranslationClass(HorInterFiller);
public:
				mDefaultFactoryInstantiation( VolProc::Step,
				    HorInterFiller, "HorInterFiller",
				    tr("Horizon-based painter - Simple") )

				~HorInterFiller();
				HorInterFiller();

    bool			isOK() const;

    bool			needsInput() const override { return false; }
    bool			isInputPrevStep() const override
				{ return true; }

    bool			setTopHorizon(const MultiID*);
    const MultiID*		getTopHorizonID() const;

    bool			setBottomHorizon(const MultiID*);
    const MultiID*		getBottomHorizonID() const;

    float			getTopValue() const;
    void			setTopValue(float);

    bool			usesGradient() const;
    void			useGradient(bool);
				//!<If false, bottom value will be used

    float			getBottomValue() const;
    void			setBottomValue(float);

    float			getGradient() const;
    void			setGradient(float);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			releaseData() override;
    bool			canInputAndOutputBeSame() const override
				{ return true; }
    bool			needsFullVolume() const override
				{ return false;}
    bool			canHandle2D() const override
				{ return true; }

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64			extraMemoryUsage(OutputSlotID,
				    const TrcKeySampling&,
				    const StepInterval<int>&) const override;

    bool		areSamplesIndependent() const override	{ return true; }

protected:
    bool			prefersBinIDWise() const override
				{ return true; }
    bool			prepareComp(int) override
				{ return true; }
    bool			computeBinID(const BinID&, int) override;

    static const char*		sKeyTopHorID()	{ return "Top horizon"; }
    static const char*		sKeyBotHorID()	{ return "Bottom horizon"; }
    static const char*		sKeyTopValue()	{ return "Top Value"; }
    static const char*		sKeyBotValue()	{ return "Bottom Value"; }
    static const char*		sKeyGradient()	{ return "Gradient"; }
    static const char*		sKeyUseGradient() { return "Use Gradient"; }


    EM::Horizon*		loadHorizon(const MultiID&) const;

    float			topvalue_;
    float			bottomvalue_;
    EM::Horizon*		tophorizon_;
    EM::Horizon*		bottomhorizon_;
    bool			usegradient_;
    float			gradient_;
};

} // namespace VolProc

