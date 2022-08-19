#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "multiid.h"

class IOObj;
class Scaler;
class SeisTrcReader;
class SeisTrcTranslator;

namespace VolProc
{

/*!
\brief Reads in a volume. Will replace previous values if data is present in
the read volume.
*/

mExpClass(VolumeProcessing) VolumeReader : public Step
{ mODTextTranslationClass(VolumeReader);
public:
			mDefaultFactoryInstantiation(
				Step, VolumeReader,
				"VolumeReader", tr("Input Volume") )
			~VolumeReader();

    bool		needsInput() const override	{ return false; }
    bool		setVolumeID(const MultiID&);
    const MultiID&	getVolumeID() const		{ return mid_; }

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    bool		canInputAndOutputBeSame() const override
			{ return true; }

    bool		needsFullVolume() const override { return false; }
    bool		canHandle2D() const override	 { return true; }

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    Task*		createTask() override;
    bool		prefersBinIDWise() const override { return false; }

    bool		prepareWork(const IOObj&);

    static const char*	sKeyVolumeID()			{ return "Volume ID"; }

    MultiID				mid_;
    ObjectSet<SeisTrcReader>		readers_;
    ObjectSet<SeisTrcTranslator>	translators_;
    TypeSet<int>			components_;
    ObjectSet<Scaler>			compscalers_;

};

} // namespace VolProc
