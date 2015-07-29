#ifndef segyuiscandata_h
#define segyuiscandata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "ranges.h"
#include "bufstring.h"

class od_istream;
class DataClipSampler;


namespace SEGY
{

class TrcHeader;
class TrcHeaderDef;


/*!\brief definition for SEG-Y scanning in UI */

mExpClass(uiSEGY) uiScanDef
{
public:

			uiScanDef();
			~uiScanDef();

    int			revision_;
    bool		hdrsswapped_;
    bool		dataswapped_;
    int			ns_;
    short		format_;
    float		coordscale_;
    SamplingData<float>	sampling_;
    TrcHeaderDef*	hdrdef_;

    void		reInit();

    TrcHeader*		getTrcHdr(od_istream&) const;

};


/*!\brief info to collect for SEG-Y scanning in UI */

mExpClass(uiSEGY) uiScanData
{
public:

			uiScanData(const char* fnm);

    const BufferString	filenm_;
    bool		usable_;

    int			nrtrcs_;
    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<float>	offsrg_;
    StepInterval<int>	inls_;
    StepInterval<int>	crls_;
    Interval<float>	refnrs_;

    StepInterval<int>&	trcNrs()		{ return crls_; }
    const StepInterval<int>& trcNrs() const	{ return crls_; }

    void		getFromSEGYBody(od_istream&,const uiScanDef&,
					DataClipSampler* cs=0);
			//!< if clipsampler is passed, will scan a lot of traces
			//!< otherwise, only a few (1st, 2nd and last)
    void		merge(const uiScanData&);

};

} // namespace SEGY


#endif
