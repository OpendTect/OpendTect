#ifndef volprocvolreader_h
#define volprocvolreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: volprocvolreader.h,v 1.4 2010-05-11 13:55:05 cvsyuancheng Exp $
________________________________________________________________________

-*/

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
    
mClass VolumeReader : public Step
{
public:
    static void		initClass();
    
    			~VolumeReader();
			VolumeReader(Chain&);

    bool		isOK() const;

    const char*		type() const { return sKeyType(); }
    bool		needsInput(const HorSampling&) const	{ return true; }			
    bool 		setVolumeID(const MultiID&);
    const MultiID&	getVolumeID() const			{ return mid_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    void		releaseData();
    bool		canInputAndOutputBeSame() const		{ return true; }
    bool		needsFullVolume() const 		{ return false;}

    static const char*	sKeyType()  { return "VolumeReader"; }
    static const char*	sUserName() { return "Stored Volume"; }
    
protected:
    bool			prefersBinIDWise() const        { return true; }
    bool			computeBinID(const BinID&, int);
    bool                        prepareComp(int nrthreads);

    static const char*		sKeyVolumeID()	{ return "Volume ID"; }
    bool			addReader();
  
    static Step*      		create(Chain&);

    MultiID				mid_;
    ObjectSet<SeisTrcReader>		readers_;
    ObjectSet<SeisTrcTranslator>	translators_;
};

}; //namespace


#endif
