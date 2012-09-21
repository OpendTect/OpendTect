#ifndef volprocfaultangle_h
#define volprocfaultangle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		Aug 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "multiid.h"
#include "volprocchain.h"

class BinID;

namespace VolProc
{
/*! Calculate Azimuth or Dip for a volume. */

    
mClass(VolumeProcessing) FaultAngle : public Step
{
public:
				mDefaultFactoryInstantiation( VolProc::Step,
				    FaultAngle, "FaultAngle",
				    "Fault Orientation" );

    				~FaultAngle();
				FaultAngle();

    enum OutputOption		{ FaultFlag=0, Azimuth=1, Dip=2 };
    void			setOutputOption(OutputOption oo) { outopt_=oo;}
    OutputOption		getutputOption() const	{ return outopt_; }    

    void			setMinFaultLength(int nr) { minlength_ = nr; }
    int				minFaultLength() const	  { return minlength_; }
    void			doThinning(bool yn)	{ dothinning_ = yn; }
    bool			doThinning() const	{ return dothinning_; }
    void			doMerge(bool yn)	{ domerge_ = yn; }
    bool			doMerge() const		{ return domerge_; }
    void			overlapRate(float r)	{ overlaprate_ = r; }
    float			overlapRate() const	{ return overlaprate_; }
    void			threshold(float t)	{ fltthreshold_ = t; }
    float			threshold() const	{ return fltthreshold_;}
    void			isFltAbove(bool yn)	{ isfltabove_ = yn; }
    bool			isFltAbove() const	{ return isfltabove_; }

    bool			isOK() const;
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
   
    bool			needsInput() const 		{ return false;}
    bool			canInputAndOutputBeSame() const { return true; }
    bool			needsFullVolume() const		{ return true;}

    
protected:
    bool			prefersBinIDWise() const        { return true; }
    bool                        prepareComp(int)		{ return true; }
    bool			computeBinID(const BinID&, int);
    
    OutputOption		outopt_;
    bool			isdone_;
    int				minlength_;
    bool			dothinning_;
    bool			domerge_;
    float			overlaprate_;
    float			fltthreshold_;
    bool			isfltabove_;

    static const char*		sKeyOutputOpt()	{ return "Output option"; }
    static const char*		sKeyFltLength()	{ return "Fault min length"; }
    static const char*		sKeyThinning()	{ return "Do thinning"; }
    static const char*		sKeyMerge()	{ return "Do mgerge"; }
    static const char*		sKeyOverlapRate() { return "Overlap rate"; }
    static const char*		sKeyThreshold()	{ return "Fault threshold"; }
    static const char*		sKeyIsAbove()	{ return "Fault is above"; }
};

}; //namespace


#endif

