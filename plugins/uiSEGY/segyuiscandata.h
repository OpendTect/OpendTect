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
#include "datachar.h"

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

    bool		isValid() const			{ return ns_ > 0; }
    int			bytesPerSample() const;
    int			traceDataBytes() const;
    DataCharacteristics	getDataChar() const;

    int			nrTracesIn(const od_istream&,od_stream_Pos p=-1) const;
    void		goToTrace(od_istream&,od_stream_Pos,int) const;
    TrcHeader*		getTrcHdr(od_istream&) const;
    bool		getData(od_istream&,char*,float* vals=0) const;
    TrcHeader*		getTrace(od_istream&,char*,float*) const;

    void		reInit();

};


/*!\brief info to collect for SEG-Y scanning in UI */

mExpClass(uiSEGY) uiScanData
{
public:

			uiScanData(const char* fnm);

    BufferString	filenm_;
    bool		usable_;

    int			nrtrcs_;
    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<float>	offsrg_;
    Interval<int>	inls_;
    Interval<int>	crls_;
    Interval<int>	trcnrs_;
    Interval<float>	refnrs_;

    void		getFromSEGYBody(od_istream&,const uiScanDef&,
				    bool isfirst,bool is2d,DataClipSampler&);
    void		merge(const uiScanData&);

    void		reInit();

protected:

    bool		addTrace(od_istream&,bool,char*,float*,const uiScanDef&,
				 DataClipSampler&);
    void		addValues(DataClipSampler&,const float*, int);

};

} // namespace SEGY


#endif
