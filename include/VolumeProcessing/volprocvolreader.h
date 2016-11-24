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
#include "dbkey.h"

class IOObj;
class SeisTrcReader;
class SeisTrcTranslator;


namespace VolProc
{

/*!\brief Reads a volume. Will replace previous values if data is present in
	    the read volume. */

mExpClass(VolumeProcessing) VolumeReader : public Step
{ mODTextTranslationClass(VolumeReader);
public:
				mDefaultFactoryInstantiation(
					Step, VolumeReader,
					"VolumeReader", tr("Input Volume") )

    bool			setVolumeID(const DBKey&);
    const DBKey&		getVolumeID() const		{ return mid_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    virtual Task*		createTask();

    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const	{ return true; }
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		canHandle2D() const		{ return true; }
    virtual bool		needsInput() const		{ return false;}

protected:

    virtual od_int64		extraMemoryUsage(OutputSlotID,
					 const TrcKeySampling&,
					 const StepInterval<int>&) const;

    bool			prepareWork(const IOObj&);

    static const char*		sKeyVolumeID()		{ return "Volume ID"; }

    DBKey			mid_;
    TypeSet<int>		components_;
    ObjectSet<SeisTrcReader>	readers_;
    ObjectSet<SeisTrcTranslator> translators_;

};

} // namespace VolProc
