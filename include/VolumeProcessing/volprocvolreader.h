#ifndef volprocvolreader_h
#define volprocvolreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"

class SeisTrcReader;
class SeisTrcTranslator;
class BinID;

namespace VolProc
{

/*! Reads in a volume. Will replace previous values if data is present
    in the read volume. */
    
mClass(VolumeProcessing) VolumeReader : public Step
{
public:
    			mDefaultFactoryInstantiation( Step, VolumeReader,
				"VolumeReader", "Stored Volume" );
    			~VolumeReader();

    bool		needsInput() const			{ return false;}
    bool 		setVolumeID(const MultiID&);
    const MultiID&	getVolumeID() const			{ return mid_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		canInputAndOutputBeSame() const		{ return true; }
    bool		needsFullVolume() const 		{ return false;}

protected:
    Task*			createTask();

    bool			prefersBinIDWise() const        { return false; }

    static const char*		sKeyVolumeID()	{ return "Volume ID"; }

    MultiID				mid_;
    ObjectSet<SeisTrcReader>		readers_;
    ObjectSet<SeisTrcTranslator>	translators_;
};

}; //namespace


#endif

