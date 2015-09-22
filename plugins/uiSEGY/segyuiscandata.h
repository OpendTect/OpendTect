#ifndef segyuiscandata_h
#define segyuiscandata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "ranges.h"
#include "bufstring.h"
#include "datachar.h"

class SeisTrcInfo;
class DataClipSampler;
class uiParent;
namespace PosInfo { class Detector; }


namespace SEGY
{

class TrcHeader;
class TrcHeaderDef;
class OffsetCalculator;


/*!\brief Basic data from a SEG-Y fle */

mExpClass(uiSEGY) BasicFileInfo
{
public:

			BasicFileInfo()			{ init(); }
    void		init();

    int			revision_;
    int			ns_;
    short		format_;
    SamplingData<float>	sampling_;
    bool		hdrsswapped_;
    bool		dataswapped_;

    bool		isValid() const			{ return ns_ > 0; }
    bool		isRev0() const			{ return revision_ < 1;}
    int			bytesPerSample() const;
    int			traceDataBytes() const;
    DataCharacteristics	getDataChar() const;
    StepInterval<float>	getZRange() const
			{ return sampling_.interval(ns_); }

    int			nrTracesIn(const od_istream&,od_stream_Pos p=-1) const;
    bool		goToTrace(od_istream&,od_stream_Pos,int) const;

};


/*!\brief definition for SEG-Y loading */

mExpClass(uiSEGY) LoadDef : public BasicFileInfo
{
public:

			LoadDef();
			~LoadDef();
    void		reInit(bool alsohdef);

    float		coordscale_;
    FileReadOpts::PSDefType psoffssrc_;
    FileReadOpts::ICvsXYType icvsxytype_;
    SamplingData<float>	psoffsdef_;

    TrcHeaderDef*	hdrdef_;

    TrcHeader*		getTrcHdr(od_istream&) const;
    bool		getData(od_istream&,char*,float* vals=0) const;
    TrcHeader*		getTrace(od_istream&,char*,float*) const;
    bool		skipData(od_istream&) const;
    void		getTrcInfo(TrcHeader&,SeisTrcInfo&,
	    			   const OffsetCalculator&) const;

    bool		findRev0Bytes(od_istream&);

};


/*!\brief info collected by scanning SEG-Y file */

mExpClass(uiSEGY) ScanInfo
{
public:

			ScanInfo(const char* fnm);

    BufferString	filenm_;
    bool		usable_;
    bool		fullscan_;
    bool		isEmpty() const		{ return nrtrcs_ < 1; }
    bool		isUsable() const	{ return usable_ && !isEmpty();}

    int			nrtrcs_;
    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<float>	offsrg_;
    Interval<int>	inls_;
    Interval<int>	crls_;
    Interval<int>	trcnrs_;
    Interval<float>	refnrs_;
    bool		infeet_;

    BasicFileInfo	basicinfo_;

    void		getFromSEGYBody(od_istream&,const LoadDef&,
				    bool ismulti,bool is2d,DataClipSampler&,
				    bool full,PosInfo::Detector*,uiParent*);
    void		merge(const ScanInfo&);

    void		reInit();

protected:

    od_stream_Pos	startpos_;

    void		addPositions(const SeisTrcInfo&,PosInfo::Detector*);
    void		addValues(DataClipSampler&,const float*,int);
    void		addTraces(od_istream&,int trcidx,bool is2d,
				  char*,float*,const LoadDef&,DataClipSampler&,
				  const OffsetCalculator&,PosInfo::Detector*,
				  bool rev=false);

    friend class	FullUIScanner;

};

} // namespace SEGY


#endif
