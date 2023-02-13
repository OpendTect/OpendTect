#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "ranges.h"
#include "bufstring.h"
#include "datachar.h"
#include "namedobj.h"
#include "survinfo.h"
#include "refcount.h"

class SeisTrcInfo;
class DataClipSampler;
class uiParent;
class TaskRunner;
namespace Coords  { class CoordSystem; }
namespace PosInfo { class Detector; }


namespace SEGY
{

class FilePars;
class TrcHeader;
class TrcHeaderDef;
class FileReadOpts;
class HdrEntryKeyData;
class OffsetCalculator;


/*!\brief Basic data from a SEG-Y fle */

mExpClass(uiSEGYTools) BasicFileInfo
{ mODTextTranslationClass(BasicFileInfo);
public:

			BasicFileInfo(bool is2d);
			BasicFileInfo(const BasicFileInfo&);
    virtual		~BasicFileInfo();

    BasicFileInfo&	operator =(const BasicFileInfo&);
    void		init(bool is2d);

    int			revision_;
    int			binsr_;		// sample rate (us or mm)
    int			ns_;		// nr samples binary header
    int			thdrns_;	// nr samples trace header
    short		format_;
    SamplingData<float>	sampling_;
    bool		hdrsswapped_;
    bool		dataswapped_;
    bool		is2d_;
    RefMan<Coords::CoordSystem> coordsystem_;

    bool		usenrsampsinfile_	= true;
    bool		useformatinfile_	= true;

    bool		isValid() const			{ return ns_ > 0; }
    bool		isRev0() const			{ return revision_ < 1;}
    int			bytesPerSample() const;
    int			traceDataBytes() const;
    DataCharacteristics	getDataChar() const;
    StepInterval<float>	getZRange() const
			{ return sampling_.interval(ns_); }

    TrcHeader*		getTrcHdr(od_istream&) const;
    int			nrTracesIn(const od_istream&,od_stream_Pos p=-1) const;
    bool		goToTrace(od_istream&,od_stream_Pos,int) const;
    uiString		getFrom(od_istream&,bool& zinft,
				const bool* knownhdrswap =nullptr);
			//!< try returned isEmpty(), if not error occurred
			//!< on success leaves stream at start of first trace

    virtual void	getFilePars(FilePars&) const;

protected:

    virtual const TrcHeaderDef& getHDef() const; // returns default

};


/*!\brief definition for SEG-Y loading */

mExpClass(uiSEGYTools) LoadDef : public BasicFileInfo
{
public:

			LoadDef(bool is2d);
			LoadDef(const LoadDef&);
			~LoadDef();

    LoadDef&		operator =(const LoadDef&);
    void		reInit(bool is2d,bool alsohdef);

    float		coordscale_;
    FileReadOpts::ICvsXYType icvsxytype_;
    bool		havetrcnrs_;
    SamplingData<int>	trcnrdef_;
    FileReadOpts::PSDefType psoffssrc_;
    SamplingData<float>	psoffsdef_;
    bool		usezsamplinginfile_;

    TrcHeaderDef*	hdrdef_ = nullptr;

    LoadDef		getPrepared(od_istream&) const;
    bool		getData(od_istream&,char*,float* vals=0) const;
    TrcHeader*		getTrace(od_istream&,char*,float*) const;
    bool		skipData(od_istream&) const;
    void		getTrcInfo(TrcHeader&,SeisTrcInfo&,
				   const OffsetCalculator&) const;

    virtual void	getFilePars(FilePars&) const;
    void		getFileReadOpts(FileReadOpts&) const;
    void		usePar(const IOPar&);
    void		setUserCoordSys(Coords::CoordSystem* crs)
			{ coordsys_ = crs; }
    ConstRefMan<Coords::CoordSystem>	   getUserCoordSys()
			      { return coordsys_; }
    bool		needXY() const;

protected:

    virtual const TrcHeaderDef& getHDef() const	{ return *hdrdef_; }
    ConstRefMan<Coords::CoordSystem> coordsys_;

};


/*!\brief range info collected by scanning SEG-Y file */

mExpClass(uiSEGYTools) ScanRangeInfo
{
public:
			ScanRangeInfo()		{ reInit(); }

    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<int>	inls_;
    Interval<int>	crls_;
    Interval<int>	trcnrs_;
    Interval<float>	refnrs_;
    Interval<float>	offs_;
    Interval<float>	azims_;

    void		reInit();
    void		use(const PosInfo::Detector&);
    void		merge(const ScanRangeInfo&);
};


/*!\brief info collected by scanning a SEG-Y file */

mExpClass(uiSEGYTools) ScanInfo
{
public:

			ScanInfo(const char* fnm,bool is2d);
			~ScanInfo();

    BasicFileInfo&	basicInfo()		{ return basicinfo_; }

    void		getFromSEGYBody(od_istream&,const LoadDef&,bool surv,
				    DataClipSampler&,TaskRunner* t=0);
			//!< will do full scan if TaskRunner passed

    void		merge(const ScanInfo&);

    const char*		fileName() const	{ return filenm_; }
    bool		is2D() const;
    bool		isEmpty() const		{ return nrtrcs_ < 1; }
    int			nrTraces() const	{ return nrtrcs_; }
    bool		isFull() const		{ return full_; }

    const HdrEntryKeyData&	keyData() const		{ return keydata_; }
    const ScanRangeInfo&	ranges() const		{ return rgs_; }
    const PosInfo::Detector&	piDetector() const	{ return *pidetector_; }
    const BasicFileInfo&	basicInfo() const	{ return basicinfo_; }

protected:

    BufferString	filenm_;
    PosInfo::Detector*	pidetector_ = nullptr;
    HdrEntryKeyData&	keydata_;
    BasicFileInfo	basicinfo_;
    ScanRangeInfo	rgs_;
    int			nrtrcs_;
    int			idxfirstlive_;
    bool		full_;

    od_stream_Pos	startpos_;

    void		reInit()		{ init( is2D() ); }
    void		addTrace(TrcHeader&,const float*,const LoadDef&,
				 DataClipSampler&,const OffsetCalculator&,
				 int trcidx);
    void		addTraces(od_istream&,Interval<int>,char*,float*,
				  const LoadDef&,DataClipSampler&,
				  const OffsetCalculator&);
    bool		addNextTrace(od_istream&,char*,float*,
					 const LoadDef&,DataClipSampler&,
					 const OffsetCalculator&);
    void		ensureStepsFound(od_istream&,char*,float*,
					 const LoadDef&,DataClipSampler&,
					 const OffsetCalculator&);
    void		addValues(DataClipSampler&,const float*,int);

private:

    void		init(bool);
    void		finishGet(od_istream&);

    friend class	FullUIScanner;

};


/*!\brief set of SEG-Y Scan Infos */

mExpClass(uiSEGYTools) ScanInfoSet : public NamedObject
{
public:

			ScanInfoSet(bool is2d,bool isps);
			~ScanInfoSet();

    void		setEmpty();
    ScanInfo&		add(const char* fnm);	//!< does not open anything
    void		removeLast();
    void		setInFeet( bool yn )	{ infeet_ = yn; }
    void		finish();

    bool		is2D() const		{ return is2d_; }
    bool		isEmpty() const		{ return size() < 1; }
    int			nrTraces() const	{ return nrtrcs_; }
    bool		inFeet() const		{ return infeet_; }
    bool		isFull() const;
    const BasicFileInfo& basicInfo() const;
    const PosInfo::Detector& piDetector() const	{ return detector_; }
    const HdrEntryKeyData& keyData() const	{ return keydata_; }
    const ScanRangeInfo& ranges() const		{ return rgs_; }

    int			size() const		{ return sis_.size(); }
    const ScanInfo&	scanInfo( int i ) const	{ return *sis_[i]; }

protected:

    ObjectSet<ScanInfo>	sis_;
    bool		is2d_;
    bool		isps_;
    bool		infeet_;
    int			nrtrcs_;
    HdrEntryKeyData&	keydata_;
    ScanRangeInfo	rgs_;
    PosInfo::Detector&	detector_;

};

} // namespace SEGY
