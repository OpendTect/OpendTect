#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
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

    bool		needsInput() const		{ return false; }
    bool		setVolumeID(const MultiID&);
    const MultiID&	getVolumeID() const		{ return mid_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		needsFullVolume() const		{ return false; }
    bool		canHandle2D() const		{ return true; }

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

protected:

    Task*		createTask();
    bool		prefersBinIDWise() const        { return false; }

    bool		prepareWork(const IOObj&);

    static const char*	sKeyVolumeID()			{ return "Volume ID"; }

    MultiID				mid_;
    ObjectSet<SeisTrcReader>		readers_;
    ObjectSet<SeisTrcTranslator>	translators_;
    TypeSet<int>			components_;
    ObjectSet<Scaler>			compscalers_;

};

} // namespace VolProc

