#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "executor.h"
#include "paralleltask.h"

#include "bufstringset.h"
#include "enums.h"
#include "position.h"
#include "ranges.h"
#include "stattype.h"
#include "uistrings.h"
#include "welldata.h"

class DataPointSet;
class IODirEntryList;
class IODir;
class IOObj;
class SurveyChanger;
template <class T> class Array2D;


namespace Well
{
class D2TModel;
class Info;
class LoadReqs;
class Log;
class LogSet;
class Marker;
class MarkerSet;
class Track;

/*!
\brief Parameters (zrg, sampling method) to extract well data.
*/

mExpClass(Well) ZRangeSelector
{ mODTextTranslationClass(ZRangeSelector);
public :
			ZRangeSelector();
			ZRangeSelector(const ZRangeSelector&);
    virtual		~ZRangeSelector();

    enum		ZSelection { Markers, Depths, Times };
			mDeclareEnumUtils(ZSelection);

    ZSelection		zselection_		= Markers;

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
    virtual bool	isOK(uiString* errmsg=nullptr) const;
    virtual void	fill(LoadReqs&) const;

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
				 uiString& errmsg,bool todah=true) const;
    Interval<float>	calcFrom(const Data&,const Well::LogSet& logset,
				 uiString& errmsg,bool todah=true) const;

    float		topOffset() const	{ return above_; }
    float		botOffset() const	{ return below_; }
    const char*		topMarker() const	{ return topmrkr_; }
    const char*		botMarker() const	{ return botmrkr_; }
    Interval<float>	getFixedRange() const	{ return fixedzrg_; }
    bool		isInTime() const	{ return zselection_ == Times; }

protected:

    Interval<float>	fixedzrg_		= Interval<float>::udf();
    BufferString	topmrkr_		= sKeyDataStart();
    BufferString	botmrkr_		= sKeyDataEnd();
    float		above_			= 0.f;
    float		below_			= 0.f;
    bool		snapzrgtosurvey_	= false;

    void		setMarker(bool top,const char* nm,float offset);
    void		getMarkerRange(const Data&,
				       Interval<float>&) const;
    void		getLimitPos(const MarkerSet&,bool,float&,
				    const Interval<float>&) const;
    void		snapZRangeToSurvey(Interval<float>&,bool zistime,
					  const D2TModel*,
					  const Track&) const;
};


/*!
\brief ZRangeSelector to extract parameters.
*/

mExpClass(Well) ExtractParams : public ZRangeSelector
{ mODTextTranslationClass(ExtractParams);
public:
			ExtractParams();
			ExtractParams(const ExtractParams&);
			~ExtractParams();

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;

    void		setEmpty() override;
    bool		isOK(uiString* errmsg=nullptr) const override;

    static const char*	sKeySamplePol();
    static const char*	sKeyZExtractInTime();
    float		getZStep() const;

    float		zstep_			= 1.f; //can be in time
    bool		extractzintime_		= false;
    Stats::UpscaleType	samppol_		= Stats::UseAvg;
};


/*!
\brief Collects information about all wells in store.
*/

mExpClass(Well) InfoCollector : public SequentialTask
{ mODTextTranslationClass(InfoCollector);
public:

			InfoCollector(bool wellloginfo=true,
				      bool markers=true,
				      bool trackinfo=false);
			~InfoCollector();

    uiString		uiMessage() const override	{ return curmsg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Wells inspected"); }

    const TypeSet<MultiID>&	ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return logs_; }
				//!< If selected, same size as ids()
    const Interval<float>	getTracksTVDRange() const {return trackstvdrg_;}

    void			getAllMarkerInfos(BufferStringSet& nms,
					    TypeSet<OD::Color>& colors) const;
    void			getAllLogNames(BufferStringSet&) const;
    void			setSurvey(const SurveyDiskLocation&);
    SurveyDiskLocation&		survey() const;

protected:
    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool,od_ostream* =nullptr) override;

    od_int64		nrDone() const override		{ return curidx_; }
    od_int64		totalNr() const override	{ return totalnr_; }

    SurveyDiskLocation&		survloc_;
    SurveyChanger*		chgr_		= nullptr;
    TypeSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_	= nullptr;
    const IODir*		iodir_		= nullptr;
    int				totalnr_;
    int				curidx_		= 0;
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

mExpClass(Well) TrackSampler : public ::SequentialTask
{ mODTextTranslationClass(TrackSampler);
public:

			TrackSampler(const TypeSet<MultiID>& ioobjids,
				     ObjectSet<DataPointSet>&,
				     bool zvalsintime);
			~TrackSampler();

    float		locradius_	= 0.f;
    bool		for2d_		= false;
    bool		minidps_	= false;
    bool		mkdahcol_	= false;
    BufferStringSet	lognms_;

    ExtractParams	params_;

    void		usePar(const IOPar&);

    uiString		uiMessage() const override
			{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Wells inspected"); }

    static const char*	sKeySelRadius();
    static const char*	sKeyDahCol();
    static const char*	sKeyFor2D();
    static const char*	sKeyLogNm();

private:

    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream* =nullptr) override;

    od_int64		nrDone() const override    { return curid_; }
    od_int64		totalNr() const override   { return ids_.size(); }

    const TypeSet<MultiID>&	ioObjIds() const	{ return ids_; }
    ObjectSet<DataPointSet>&	dataPointSets()		{ return dpss_; }

    const TypeSet<MultiID>&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_		= 0;
    const bool			zistime_;
    Interval<float>		zrg_;
    int				dahcolnr_	= -1;
    uiString			msg_;

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

			LogDataExtracter(const TypeSet<MultiID>& ioobjids,
					 ObjectSet<DataPointSet>&,
					 bool zvalsintime);
			~LogDataExtracter();

    BufferString	lognm_;
    Stats::UpscaleType	samppol_;
    static const char*	sKeyLogNm(); //!< equals address of TrackSampler's

    void		usePar(const IOPar&);

    int			nextStep() override;
    uiString		uiMessage() const override   { return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Wells handled"); }

    od_int64		nrDone() const override    { return curid_; }
    od_int64		totalNr() const override   { return ids_.size(); }

    const TypeSet<MultiID>&	ioObjIds() const	{ return ids_; }

    static float	calcVal(const Log&,float dah,float winsz,
				Stats::UpscaleType samppol,
				float maxholesz=mUdf(float),
				bool logisvel=false);

protected:

    const TypeSet<MultiID>&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    uiString			msg_;

    void		getData(DataPointSet&,const Data&,const Track&);
    void		getGenTrackData(DataPointSet&,const Track&,const Log&,
					int,int);
    void		addValAtDah(float,const Log&,float,
				    DataPointSet&,int,int) const;
    float		findNearest(const Track&,const BinIDValue&,
				    float,float,float) const;
};


/*!
\brief Executor to sample Well::Track
*/

mExpClass(Well) SimpleTrackSampler : public Executor
{ mODTextTranslationClass(SimpleTrackSampler);
public:
			SimpleTrackSampler(const Track&, const D2TModel*,
					   bool extrapolate_ = false,
					   bool stayinsidesurvey = false);
			~SimpleTrackSampler();

    void		setSampling(const StepInterval<float>& intv)
			{ extrintv_ = intv; } //In time if d2TModel is provided

    int			nextStep() override;
    od_int64		totalNr() const override { return extrintv_.nrSteps(); }
    od_int64		nrDone() const override { return nrdone_; }
    uiString		uiMessage() const override
			{ return m3Dots(tr("Computing")); }
    uiString		uiNrDoneText() const override
			{ return tr("Points done"); }

    void		getBIDs(TypeSet<BinID>& bs) const { bs = bidset_; }
    void		getCoords(TypeSet<Coord>& cs) const { cs = coords_; }

protected:
    StepInterval<float> extrintv_;

    TypeSet<BinID>	bidset_;
    TypeSet<Coord>	coords_;

    bool		isinsidesurvey_;
    bool		extrapolate_;

    Interval<float>	tracklimits_;
    const Track&	track_;
    const D2TModel*	d2t_;
    int			nrdone_;
};


/*!
\brief Log resampler, extracts all the logs given by log names along a z time
or dah axis.
*/

mExpClass(Well) LogSampler : public ParallelTask
{ mODTextTranslationClass(LogSampler);
public:
			LogSampler(const Data&,const Well::ExtractParams&,
				   const BufferStringSet& lognms);

			LogSampler(const Data&, const Interval<float>& zrg,
				   bool zrgintime,float zstep,
				   bool extractintime,
				   Stats::UpscaleType samppol,
				   const BufferStringSet& lognms);

			LogSampler(const D2TModel* d2t, const Track* track,
				const Interval<float>& zrg, bool zrgintime,
				float zstep, bool extractintime,
				Stats::UpscaleType samppol,
				const ObjectSet<const Well::Log>& logs);

			LogSampler(const Data&, const ExtractParams&,
				   const LogSet&, const BufferStringSet&);

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
    const char*		uomLabel(int logidx) const;
			//!< Unit of Measure label

    uiString		errMsg() const
			{ return errmsg_.isEmpty() ? uiString::empty()
						   : errmsg_; }

    int			nrZSamples() const;
    Interval<float>	zRange() const	{ return zrg_; } //can be in time

    uiString		uiNrDoneText() const override;

protected:
    void		init (const D2TModel*,const Interval<float>&,
			    bool zrgintime,float zstep, bool extractintime,
			    Stats::UpscaleType samppol);

    od_int64		nrIterations() const override;

    bool		doLog(int logidx);
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;

    ConstRefMan<Data>		wd_;
    const D2TModel*		d2t_;
    const Track&		track_;
    Interval<float>		zrg_;
    float			zstep_;
    bool			extrintime_;
    bool			zrgisintime_;
    float			maxholesz_	= mUdf(float);
    ObjectSet<const Log>	logset_;
    Array2D<float>*		data_		= nullptr;
    uiString			errmsg_;
    Stats::UpscaleType		samppol_;
};

} // namespace Well
