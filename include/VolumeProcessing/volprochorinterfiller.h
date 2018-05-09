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
namespace EM { class Horizon; }

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
				mDefaultFactoryInstantiation( Step,
				    HorInterFiller, "HorInterFiller",
				    tr("Horizon-based painter - Simple") )

				HorInterFiller();
				~HorInterFiller();
    virtual void		releaseData();

    bool			isOK() const;

    bool			setTopHorizon(const DBKey*);
    const DBKey*		getTopHorizonID() const;

    bool			setBottomHorizon(const DBKey*);
    const DBKey*		getBottomHorizonID() const;

    float			getTopValue() const;
    void			setTopValue(float);

    bool			usesGradient() const;
    void			useGradient(bool);
				//!<If false, bottom value will be used

    float			getBottomValue() const;
    void			setBottomValue(float);

    float			getGradient() const;
    void			setGradient(float);

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

private:

    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const	{ return true; }
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		needsInput() const		{ return false;}
    virtual bool		isInputPrevStep() const		{ return true; }
    virtual bool		canHandle2D() const		{ return true; }
    virtual bool		prefersBinIDWise() const        { return true; }

    virtual bool		computeBinID(const BinID&, int);
    virtual od_int64		extraMemoryUsage(OutputSlotID,
					const TrcKeyZSampling&) const;

    EM::Horizon*		loadHorizon(const DBKey&) const;

    float			topvalue_;
    float			bottomvalue_;
    EM::Horizon*		tophorizon_;
    EM::Horizon*		bottomhorizon_;
    bool			usegradient_;
    float			gradient_;

    static const char*		sKeyTopHorID()	{ return "Top horizon"; }
    static const char*		sKeyBotHorID()	{ return "Bottom horizon"; }
    static const char*		sKeyTopValue()	{ return "Top Value"; }
    static const char*		sKeyBotValue()	{ return "Bottom Value"; }
    static const char*		sKeyGradient()  { return "Gradient"; }
    static const char*		sKeyUseGradient() { return "Use Gradient"; }

};

} // namespace VolProc
