#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2004
________________________________________________________________________

-*/

#include "wellcommon.h"
#include "executor.h"
#include "paralleltask.h"
#include "bufstringset.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
#include "stattype.h"
#include "survinfo.h"
#include "uistrings.h"
#include "dbkey.h"

class DataPointSet;
class IOObj;
class DBDir;
class DBDirEntryList;
class UnitOfMeasure;
template <class T> class Array2DImpl;


namespace Well
{

/*!\brief Parameters (zrg, sampling method) to extract well data.  */

mExpClass(Well) ZRangeSelector
{ mODTextTranslationClass(ZRangeSelector);
public:

    typedef float	ZType;

			ZRangeSelector() { setEmpty(); }
			ZRangeSelector(const ZRangeSelector&);
    virtual		~ZRangeSelector() {}


    enum		ZSelection { Markers, Depths, Times };
			mDeclareEnumUtils(ZSelection);

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
    virtual bool	isOK(uiString* errmsg=0) const;

    //set
    void		setTopMarker(const char* nm,ZType offset)
			{ setMarker( true, nm, offset); }
    void		setBotMarker(const char* nm,ZType offset)
			{ setMarker( false, nm, offset); }
    void		setFixedRange(Interval<ZType>,bool istime);
    void		snapZRangeToSurvey(bool yn)
			{ snapzrgtosurvey_ = yn; }

    //get
    Interval<ZType>	calcFrom(const Data&,const BufferStringSet& logs,
				 bool todah=true) const;

    ZType		topOffset() const	{ return above_; }
    ZType		botOffset() const	{ return below_; }
    const char*		topMarker() const	{ return topmrkr_; }
    const char*		botMarker() const	{ return botmrkr_; }
    Interval<ZType>	getFixedRange() const	{ return fixedzrg_; }
    void		getDahRange(const Data&,Interval<ZType>&);

    bool		isInTime() const	{ return zselection_ == Times; }
    virtual ZRangeSelector* clone() const;

protected:

    Interval<ZType>	fixedzrg_;
    BufferString	topmrkr_;
    BufferString	botmrkr_;
    ZType		above_;
    ZType		below_;
    bool		snapzrgtosurvey_;

    void		setMarker(bool top,BufferString nm,ZType offset);
    void		getMarkerRange(const Data&,Interval<ZType>&) const;
    void		getLimitPos(const MarkerSet&,bool,ZType&,
				    const Interval<ZType>&) const;
    void		snapZRangeToSurvey(Interval<ZType>&,bool,
					  const D2TModel&,
					  const Track&) const;
};


/*!\brief ZRangeSelector to extract parameters.  */

mExpClass(Well) ExtractParams : public ZRangeSelector
{ mODTextTranslationClass(ExtractParams);
public:
			ExtractParams() { setEmpty(); }
			ExtractParams(const ExtractParams&);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    void		setEmpty();
    bool		isOK(uiString* errmsg=0) const;

    static const char*	sKeySamplePol();
    static const char*	sKeyZExtractInTime();
    float		getZStep() const;

    float		zstep_; //can be in time
    bool		extractzintime_;
    Stats::UpscaleType	samppol_;
    virtual ZRangeSelector* clone() const;
};


/*!\brief Collects information about all wells in store.  */

mExpClass(Well) InfoCollector : public ::Executor
{ mODTextTranslationClass(InfoCollector);
public:

			InfoCollector(bool wellloginfo=true,
				      bool markers=true,
				      bool trackinfo=false);
			~InfoCollector();

    void		setSurvey(const SurveyDiskLocation&);

    int			nextStep();
    uiString		message() const	{ return curmsg_; }
    uiString		nrDoneText() const	{
						return tr("Wells inspected");
						}
    od_int64		nrDone() const		{ return curidx_; }
    od_int64		totalNr() const		{ return totalnr_; }

    const SurveyDiskLocation&	survey() const	{ return survloc_; }
    const DBKeySet&		ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return lognames_; }
				//!< If selected, same size as ids()
    const ObjectSet< ObjectSet<const UnitOfMeasure> >& logUnits() const
				{ return loguoms_; }
				//!< If selected, same size as ids()
    const Interval<float>	getTracksTVDRange() const {return trackstvdrg_;}

    void			getAllMarkerInfos(BufferStringSet&,
						  TypeSet<Color>&) const;
    void			getAllLogNames(BufferStringSet&) const;

protected:

    SurveyDiskLocation&		survloc_;
    DBKeySet			ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	lognames_;
    ObjectSet< ObjectSet<const UnitOfMeasure> > loguoms_;
    DBDirEntryList*		direntries_;
    int				totalnr_;
    int				curidx_;
    uiString			curmsg_;
    bool			domrkrs_;
    bool			dologs_;
    bool			dotracks_;
    Interval<float>		trackstvdrg_;

};


/*!
\brief Collects positions along selected well tracks. The DataPointSet will get
new rows with the positions along the track.
*/

mExpClass(Well) TrackSampler : public ::Executor
{ mODTextTranslationClass(TrackSampler);
public:

			TrackSampler(const DBKeySet& ioobjids,
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
    uiString		message() const { return msg_; }
    uiString		nrDoneText() const { return tr("Wells inspected"); }
    od_int64		nrDone() const	  { return curid_; }
    od_int64		totalNr() const   { return ids_.size(); }

    uiRetVal		errMsgs() const	  { return errmsgs_; }

    const DBKeySet&	ioObjIds() const	{ return ids_; }
    ObjectSet<DataPointSet>&	dataPointSets() { return dpss_; }

    static const char*	sKeySelRadius();
    static const char*	sKeyDahCol();
    static const char*	sKeyFor2D();
    static const char*	sKeyLogNm();

protected:

    const DBKeySet		ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    Interval<float>		zrg_;
    int				dahcolnr_;
    uiString			msg_;
    uiRetVal			errmsgs_;

    void		getData(const Data&,DataPointSet&);
    bool		getPos(const Data&,float,BinIDValue&,int&,
				Coord3&) const;
    void		addPosns(DataPointSet&,const BinIDValue&,
				 const Coord3&,float dah) const;
};


/*!
\brief Collects positions along selected well tracks. Will add column to the
DataPointSet.
*/

mExpClass(Well) LogDataExtracter : public ::Executor
{ mODTextTranslationClass(LogDataExtracter);
public:

			LogDataExtracter(const DBKeySet& ioobjids,
					 ObjectSet<DataPointSet>&,
					 bool zvalsintime);

    BufferString	lognm_;
    Stats::UpscaleType	samppol_;
    static const char*	sKeyLogNm(); //!< equals address of TrackSampler's

    void		usePar(const IOPar&);

    int			nextStep();
    uiString		message() const		{ return msg_; }
    od_int64		nrDone() const		{ return curid_; }
    od_int64		totalNr() const		{ return ids_.size(); }
    uiString		nrDoneText() const
			{ return uiStrings::sWellsHandled(); }

    uiRetVal		errMsgs() const		{ return errmsgs_; }

    const DBKeySet&	ioObjIds() const	{ return ids_; }

    static float	calcVal(const Log&,float dah,float winsz,
					Stats::UpscaleType samppol,
					float maxholesz=mUdf(float));

protected:

    DBKeySet			ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    uiString			msg_;
    uiRetVal			errmsgs_;

    void		getData(DataPointSet&,const Data&,const Track&);
    void		getGenTrackData(DataPointSet&,const Track&,const Log&,
					int,int);
    void		addValAtDah(float,const Log&,float,
				    DataPointSet&,int,int) const;
    float		findNearest(const Track&,const BinIDValue&,
				    float,float,float) const;
};


/*!\brief Executor to sample Well::Track */

mExpClass(Well) SimpleTrackSampler : public Executor
{ mODTextTranslationClass(SimpleTrackSampler);
public:
			SimpleTrackSampler(const Well::Track&,
					  const Well::D2TModel&,
					  bool extrapolate_ = false,
					  bool stayinsidesurvey = false);

    void	setSampling( const StepInterval<float>& intv )
					{ extrintv_ = intv; }
					    //In time if d2TModel is provided

    int		nextStep();
    od_int64	totalNr() const         { return extrintv_.nrSteps(); }
    od_int64	nrDone() const          { return nrdone_; }
    uiString	message() const		{ return uiStrings::sCalculating(); }
    uiString	nrDoneText() const	{ return uiStrings::sPointsDone(); }

    void	getBIDs(TypeSet<BinID>& bs) const { bs = bidset_; }
    void	getCoords(TypeSet<Coord>& cs) const { cs = coords_; }

protected:

    StepInterval<float> extrintv_;

    TypeSet<BinID>      bidset_;
    TypeSet<Coord>      coords_;

    bool		isinsidesurvey_;
    bool		extrapolate_;

    Interval<float>     tracklimits_;
    const Well::Track&  track_;
    MonitorLock		monlock_;
    const Well::D2TModel* d2t_;
    int                 nrdone_;
};


/*!
\brief Log resampler, extracts all the logs given by log names along a z time
or dah axis.
*/

mExpClass(Well) LogSampler : public ParallelTask
{ mODTextTranslationClass(LogSampler);
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

    void		setMaxHoleSize( float sz )	{ maxholesz_ = sz; }
			/*!< Maximum size away from depth gate to fetch
				log dat */

    //available after execution
    float		getDah(int idz) const;
    float		getLogVal(int logidx,int idz) const;
    float		getLogVal(const char* lognm,int idx) const;
    float		getThickness(int idz) const;
			//!< Vertical thickness of a sample, not along hole

    uiString		errMsg() const
			{ return errmsg_.isEmpty() ? uiString::empty()
						   : errmsg_; }

    int			nrZSamples() const;
    Interval<float>	zRange() const	{ return zrg_; } //can be in time

protected:
    void		init (const Well::D2TModel*,const Interval<float>&,
			    bool zrgintime,float zstep, bool extractintime,
			    Stats::UpscaleType samppol);

    od_int64		nrIterations() const override;

    bool		doLog(int logidx);
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;

    const Well::D2TModel*	d2t_;
    const Well::Track&		track_;
    Interval<float>		zrg_;
    float			zstep_;
    bool			extrintime_;
    bool			zrgisintime_;
    float			maxholesz_ = mUdf(float);
    ObjectSet<const Well::Log>	logset_;
    Array2DImpl<float>*		data_;
    uiString			errmsg_;
    Stats::UpscaleType		samppol_;
};

}; // namespace Well
