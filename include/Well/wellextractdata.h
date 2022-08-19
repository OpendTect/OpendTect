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
#include "position.h"
#include "ranges.h"
#include "enums.h"
#include "stattype.h"
#include "survinfo.h"
#include "uistrings.h"
#include "welldata.h"

class DataPointSet;
class IODirEntryList;
class IOObj;
class IODir;
template <class T> class Array2DImpl;


namespace Well
{
class Log;
class Info;
class D2TModel;
class Track;
class Marker;
class MarkerSet;
class LogSet;

/*!
\brief Parameters (zrg, sampling method) to extract well data.
*/

mExpClass(Well) ZRangeSelector
{ mODTextTranslationClass(ZRangeSelector);
public :
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
    Interval<float>	calcFrom(const Data&,const Well::LogSet& logset,
				 bool todah=true) const;

    float		topOffset() const	{ return above_; }
    float		botOffset() const	{ return below_; }
    const char*		topMarker() const	{ return topmrkr_; }
    const char*		botMarker() const	{ return botmrkr_; }
    Interval<float>	getFixedRange() const	{ return fixedzrg_; }
    bool		isInTime() const	{ return zselection_ == Times; }

protected:

    Interval<float>	fixedzrg_;
    BufferString	topmrkr_;
    BufferString	botmrkr_;
    float		above_;
    float		below_;
    bool		snapzrgtosurvey_;

    void		setMarker(bool top,BufferString nm,float offset);
    void		getMarkerRange(const Data&,
				       Interval<float>&) const;
    void		getLimitPos(const MarkerSet&,bool,float&,
				    const Interval<float>&) const;
    void		snapZRangeToSurvey(Interval<float>&,bool,
					  const D2TModel*,
					  const Track&) const;
};


/*!
\brief ZRangeSelector to extract parameters.
*/

mExpClass(Well) ExtractParams : public ZRangeSelector
{ mODTextTranslationClass(ExtractParams);
public:
			ExtractParams() { setEmpty(); }
			ExtractParams(const ExtractParams&);

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;

    void		setEmpty() override;
    bool		isOK(uiString* errmsg=0) const override;

    static const char*	sKeySamplePol();
    static const char*	sKeyZExtractInTime();
    float		getZStep() const;

    float		zstep_; //can be in time
    bool		extractzintime_;
    Stats::UpscaleType	samppol_;
};


/*!
\brief Collects information about all wells in store.
*/

mExpClass(Well) InfoCollector : public ::Executor
{ mODTextTranslationClass(InfoCollector);
public:

			InfoCollector(bool wellloginfo=true,
				      bool markers=true,
				      bool trackinfo=false);
			~InfoCollector();

    int			nextStep() override;
    uiString		uiMessage() const override	{ return curmsg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Wells inspected"); }

    od_int64		nrDone() const override		{ return curidx_; }
    od_int64		totalNr() const override	{ return totalnr_; }

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

    SurveyDiskLocation&		survloc_;
    TypeSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_;
    const IODir*		iodir_;
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

			TrackSampler(const TypeSet<MultiID>& ioobjids,
				     ObjectSet<DataPointSet>&,
				     bool zvalsintime);

    float		locradius_;
    bool		for2d_;
    bool		minidps_;
    bool		mkdahcol_;
    BufferStringSet	lognms_;

    ExtractParams	params_;

    void		usePar(const IOPar&);

    int			nextStep() override;
    uiString		uiMessage() const override
			{ return tr("Scanning well tracks"); }

    uiString		uiNrDoneText() const override
			{ return tr("Wells inspected"); }

    od_int64		nrDone() const override    { return curid_; }
    od_int64		totalNr() const override   { return ids_.size(); }

    uiString		errMsg() const
			{ return errmsg_.isEmpty() ? uiString::emptyString()
						   : errmsg_; }

    const TypeSet<MultiID>&	ioObjIds() const	{ return ids_; }
    ObjectSet<DataPointSet>&	dataPointSets()		{ return dpss_; }

    static const char*	sKeySelRadius();
    static const char*	sKeyDahCol();
    static const char*	sKeyFor2D();
    static const char*	sKeyLogNm();

protected:

    const TypeSet<MultiID>&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    Interval<float>		zrg_;
    int				dahcolnr_;
    uiString			errmsg_;

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
    float			maxholesz_ = mUdf(float);
    ObjectSet<const Log>	logset_;
    Array2DImpl<float>*		data_;
    uiString			errmsg_;
    Stats::UpscaleType		samppol_;
};

} // namespace Well
