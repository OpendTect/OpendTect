#ifndef wellextractdata_h
#define wellextractdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
#include "stattype.h"
#include "survinfo.h"

class MultiID;
class DataPointSet;
class IODirEntryList;
class IOObj;
template <class T> class Array2DImpl;


namespace Well
{
class Log;
class LogSet;
class Info;
class D2TModel;
class Data;
class Track;
class Marker;
class MarkerSet;


/*!\brief parameters (zrg, sampling method) to extract well data */

mExpClass(Well) ZRangeSelector
{
public :
    			ZRangeSelector() { setEmpty(); }
    			ZRangeSelector(const ZRangeSelector&);


    enum		ZSelection { Markers, Depths, Times };
			DeclareEnumUtils(ZSelection);

    ZSelection		zselection_;

    static const char*	sKeyTopMrk();
    static const char*	sKeyBotMrk();
    static const char*	sKeyDataStart();
    static const char*	sKeyDataEnd();
    static const char*	sKeyLimits();
    static const char*	sKeyZSelection();
    static const char*	sKeyZRange();
    static const char*  sKeySnapZRangeToSurvey();

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    virtual void	setEmpty();
    virtual bool	isOK(BufferString* errmsg=0) const;

    //set
    void		setTopMarker(const char* nm,float offset)
			{ setMarker( true, nm, offset); } 
    void		setBotMarker(const char* nm,float offset)
			{ setMarker( false, nm, offset); } 
    void		setFixedRange(Interval<float>,bool istime);
    void		snapZRangeToSurvey(bool yn) 
    			{ snapzrgtosurvey_ = yn; }

    //get
    Interval<float>	calcFrom(const Data&,const BufferStringSet& logs,
	    				bool todah=true) const;
    Interval<float>	calcFrom(const IOObj&,const BufferStringSet& lgs,
	    				bool todah=true) const;


    float		topOffset() const 	{ return above_; }
    float		botOffset() const 	{ return below_; }
    const char*		topMarker() const 	{ return topmrkr_; }
    const char*		botMarker() const	{ return botmrkr_; }
    Interval<float> 	getFixedRange() const 	{ return fixedzrg_; }
    bool		isInTime() const 	{ return zselection_ == Times; }

protected:

    Interval<float>	fixedzrg_; 
    BufferString	topmrkr_;
    BufferString	botmrkr_;
    float		above_;
    float		below_;
    bool		snapzrgtosurvey_;

    void		setMarker(bool top,BufferString nm,float offset);
    void		getMarkerRange(const Data&,Interval<float>&) const;
    void		getLimitPos(const MarkerSet&,bool,float&,
				    const Interval<float>&) const;
    void		snapZRangeToSurvey(Interval<float>&,bool,
	    				  const D2TModel*,
					  const Track&) const;
};


mExpClass(Well) ExtractParams : public ZRangeSelector
{
public:
    			ExtractParams() { setEmpty(); }
    			ExtractParams(const ExtractParams&);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    void		setEmpty();
    bool		isOK(BufferString* errmsg=0) const;

    static const char*	sKeySamplePol();
    static const char*	sKeyZExtractInTime();
    float 		getZStep() const;

    float		zstep_; //can be in time 
    bool		extractzintime_;
    Stats::UpscaleType	samppol_;
};


/*!\brief Collects info about all wells in store */

mExpClass(Well) InfoCollector : public ::Executor
{
public:

			InfoCollector(bool wellloginfo=true,
				      bool markers=true,
				      bool trackinfo=false);
			~InfoCollector();

    int			nextStep();
    const char*		message() const		{ return curmsg_.buf(); }
    const char*		nrDoneText() const	{ return "Wells inspected"; }
    od_int64		nrDone() const		{ return curidx_; }
    od_int64		totalNr() const		{ return totalnr_; }

    const ObjectSet<MultiID>&	ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
    				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
    				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return logs_; }
    				//!< If selected, same size as ids()
    const Interval<float>	getTracksTVDRange() const {return trackstvdrg_;}

protected:

    ObjectSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_;
    int				totalnr_;
    int				curidx_;
    BufferString		curmsg_;
    bool			domrkrs_;
    bool			dologs_;
    bool			dotracks_;
    Interval<float>		trackstvdrg_;

};



/*!\brief Collects positions along selected well tracks. The DataPointSets will
  get new rows with the positions along the track. */

mExpClass(Well) TrackSampler : public ::Executor
{
public:

			TrackSampler(const BufferStringSet& ioobjids,
				     ObjectSet<DataPointSet>&,
				     bool zvalsintime);

    float		locradius_;
    bool		for2d_;
    bool		minidps_;
    bool		mkdahcol_;
    BufferStringSet	lognms_;

    ExtractParams	params_;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    od_int64		nrDone() const	   { return curid_; }
    od_int64		totalNr() const	   { return ids_.size(); }

    const char* 	errMsg() const 
			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }

    const BufferStringSet&	ioObjIds() const	{ return ids_; }
    ObjectSet<DataPointSet>&	dataPointSets()		{ return dpss_; }

    static const char*	sKeySelRadius();
    static const char*	sKeyDahCol();
    static const char*	sKeyFor2D();
    static const char*	sKeyLogNm();

protected:

    const BufferStringSet&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    Interval<float>		zrg_;
    int				dahcolnr_;
    BufferString		errmsg_;

    void		getData(const Data&,DataPointSet&);
    bool		getPos(const Data&,float,BinIDValue&,int&,
	    			Coord3&) const;
    void		addPosns(DataPointSet&,const BinIDValue&,
				 const Coord3&,float dah) const;
};


/*!\brief Collects positions along selected well tracks. Will add column
   to the DataPointSet. */

mExpClass(Well) LogDataExtracter : public ::Executor
{
public:

			LogDataExtracter(const BufferStringSet& ioobjids,
					 ObjectSet<DataPointSet>&,
					 bool zvalsintime);

    BufferString	lognm_;
    Stats::UpscaleType	samppol_;
    static const char*	sKeyLogNm(); //!< equals address of TrackSampler's

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return msg_.buf(); }
    const char*		nrDoneText() const { return "Wells handled"; }
    od_int64		nrDone() const	   { return curid_; }
    od_int64		totalNr() const	   { return ids_.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids_; }

    static float	calcVal(const Log&,float dah,float winsz,
	    				Stats::UpscaleType samppol); 

protected:

    const BufferStringSet&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    BufferString		msg_;

    void		getData(DataPointSet&,const Data&,const Track&);
    void		getGenTrackData(DataPointSet&,const Track&,const Log&,
	    				int,int);
    void		addValAtDah(float,const Log&,float,
	    			    DataPointSet&,int,int) const;
    float		findNearest(const Track&,const BinIDValue&,
	    			    float,float,float) const;
};



mExpClass(Well) SimpleTrackSampler : public Executor
{
public:
			SimpleTrackSampler(const Well::Track&,
					  const Well::D2TModel*,
					  bool extrapolate_ = false,
					  bool stayinsidesurvey = false);

    void                setSampling(const StepInterval<float>& intv)
			{ extrintv_ = intv; } //In time if d2TModel is provided

    int                 nextStep();
    od_int64            totalNr() const         { return extrintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }

    void		getBIDs(TypeSet<BinID>& bs) const { bs = bidset_; }
    void		getCoords(TypeSet<Coord>& cs) const { cs = coords_; }

protected:
    StepInterval<float> extrintv_;

    TypeSet<BinID>      bidset_;
    TypeSet<Coord>      coords_;

    bool		isinsidesurvey_;
    bool		extrapolate_;

    Interval<float>     tracklimits_;
    const Well::Track&  track_;
    const Well::D2TModel* d2t_;
    int                 nrdone_;
};



/*! brief Log resampler, extracts all the logs given by log names along a z time or dah axis !*/

mExpClass(Well) LogSampler : public ParallelTask
{
public:
			LogSampler(const Well::Data& wd,
				const Well::ExtractParams&,
				const BufferStringSet& lognms);

			LogSampler(const Well::Data& wd,
				const Interval<float>& zrg, bool zrgintime,
				float zstep, bool extractintime,
				Stats::UpscaleType samppol,
				const BufferStringSet& lognms);

			LogSampler(const Well::D2TModel* d2t,
				const Well::Track* track,
				const Interval<float>& zrg, bool zrgintime,
				float zstep, bool extractintime,
				Stats::UpscaleType samppol,
				const ObjectSet<const Well::Log>& logs);

			~LogSampler();

    //avalaible after execution
    float		getDah(int idz) const;
    float		getLogVal(int logidx,int idz) const;
    float		getLogVal(const char* lognm,int idx) const;

    const char*		errMsg() const 
			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }

    int 		nrZSamples() const;
    Interval<float> 	zRange() const 	{ return zrg_; } //can be in time

protected:
    void 		init (const Well::D2TModel*,const Interval<float>&,
	    		    bool zrgintime,float zstep, bool extractintime,
			    Stats::UpscaleType samppol);

    od_int64            	nrIterations() const;

    bool			doLog(int logidx);
    bool 			doPrepare(int);
    bool 			doWork(od_int64,od_int64,int);

    const Well::D2TModel* 	d2t_;
    const Well::Track&		track_;
    Interval<float>		zrg_;
    float			zstep_; 
    bool 			extrintime_;
    bool			zrgisintime_;
    ObjectSet<const Well::Log>	logset_;
    Array2DImpl<float>*		data_;
    BufferString		errmsg_;
    Stats::UpscaleType 		samppol_;
};

}; // namespace Well


#endif

