#ifndef volprocvolreader_h
#define volprocvolreader_h

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
				~VolumeReader();

    bool			setVolumeID(const MultiID&);
    const MultiID&		getVolumeID() const		{ return mid_; }

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

    static const char*		sKeyVolumeID()		{ return "Volume ID"; }

    MultiID			mid_;
    ObjectSet<SeisTrcReader>	readers_;
    ObjectSet<SeisTrcTranslator> translators_;

};

} // namespace VolProc

#endif
