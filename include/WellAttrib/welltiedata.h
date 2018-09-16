#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltiedata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________
-*/

#include "wellattribmod.h"
#include "notify.h"
#include "color.h"
#include "iopar.h"
#include "dbkey.h"
#include "uistring.h"
#include "welldisp.h"
#include "wellmarker.h"
#include "welltied2tmodelmanager.h"
#include "welltiesetup.h"
#include "wavelet.h"


class CtxtIOObj;
class SeisTrc;
namespace SynthSeis { class DataSet; }
class TaskRunner;

namespace Well { class Data; class Log; class LogSet; }

namespace WellTie
{

class DataPlayer;
class Setup;
class PickSetMgr;


mExpClass(WellAttrib) DispParams
{ mODTextTranslationClass(DispParams);
public:
			DispParams()
			    : ismarkerdisp_(true)
			    , isvwrmarkerdisp_(true)
			    , isvwrhordisp_(false)
			    , iszinft_(false)
			    , iszintime_(true)
			    , dispmrkfullnames_(true)
			    , disphorfullnames_(true)
			{}

    bool		ismarkerdisp_;
    bool		isvwrmarkerdisp_;
    bool		isvwrhordisp_;
    bool		dispmrkfullnames_;
    bool		disphorfullnames_;
    bool		iszinft_;
    bool		iszintime_;
    Well::MarkerDispProps mrkdisp_;
    BufferStringSet	allmarkernms_;

    static const char*	sKeyIsMarkerDisp();
    static const char*	sKeyVwrMarkerDisp();
    static const char*	sKeyVwrHorizonDisp();
    static const char*	sKeyZInFeet();
    static const char*	sKeyZInTime();
    static const char*	sKeyMarkerFullName();
    static const char*	sKeyHorizonFullName();

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


mExpClass(WellAttrib) Marker
{ mODTextTranslationClass(Marker);
public:

    typedef Well::Marker::LevelID LevelID;

			Marker(float z)
			    : zpos_(z)
			    , size_(2)	    {}

    Color		color_;
    float		zpos_;
    BufferString	name_;
    LevelID		id_;
    int			size_;

    bool		operator == ( const Marker& m ) const
				{ return m.zpos_ == zpos_; }
};


mExpClass(WellAttrib) PickData
{ mODTextTranslationClass(PickData);
public:
    TypeSet<Marker>		synthpicks_;
    TypeSet<Marker>		seispicks_;
};


mExpClass(WellAttrib) Data
{ mODTextTranslationClass(Data);
public :
				Data(const Setup&,Well::Data& wd);
				~Data();

    Well::Data*			wd_;

    Well::LogSet&		logset_;
    SeisTrc&			synthtrc_;
    SeisTrc&			seistrc_;
    RefMan<Wavelet>		initwvlt_;
    RefMan<Wavelet>		estimatedwvlt_;
    const Well::Log*		cslog_;

    const StepInterval<float>&	getTraceRange() const	{ return tracerg_; }
    const Interval<float>&	getDahRange() const	{ return dahrg_; }
    const StepInterval<float>&	getModelRange() const	{ return modelrg_; }
    const StepInterval<float>&	getReflRange() const	{ return reflrg_; }
    void			setTraceRange( const StepInterval<float>& zrg )
				{ tracerg_ = zrg; }
    void			computeExtractionRange();

    const Setup&		setup() const	{ return setup_; }
    const char*			sKeyDensity() const { return setup_.denlognm_; }
    const char*			sKeySonic() const { return setup_.vellognm_; }
    bool			isSonic() const	{ return setup_.issonic_; }

    static const char*		sKeyAI()	{ return "AI"; }
    static const char*		sKeyReflectivity() { return "Reflectivity"; }
    static const char*		sKeySynthetic() { return "Synthetic"; }
    static const char*		sKeySeismic()	{ return "Seismic"; }
    static float		cDefSeisSr();

    TypeSet<Marker>		horizons_;
    PickData			pickdata_;
    DispParams			dispparams_;
    TaskRunner*			trunner_;
    RefMan<SynthSeis::DataSet>	sd_;

    mStruct(WellAttrib) CorrelData
    {
				CorrelData() : lag_(200), coeff_(0) {}

	TypeSet<float>		vals_;
	int			lag_;
	double			coeff_;
	float			scaler_;
    };
    CorrelData			correl_;

protected:

    StepInterval<float>		tracerg_;
    Interval<float>		dahrg_;
    StepInterval<float>		modelrg_;
    StepInterval<float>		reflrg_;
    const Setup			setup_;
};


mExpClass(WellAttrib) DataWriter
{ mODTextTranslationClass(DataWriter);
public:
				DataWriter(Well::Data&,const DBKey&);

    bool			writeD2TM() const;
    bool			writeLogs(const Well::LogSet&,
					  bool todisk) const;
    bool			removeLogs(const Well::LogSet&) const;

    void			setWD( Well::Data* wd ) { wd_ = wd; }
    const uiString&		errMsg() const		{ return errmsg_; }

protected:

    Well::Data*			wd_;
    const DBKey&		wellid_;
    mutable uiString		errmsg_;
};


mExpClass(WellAttrib) HorizonMgr
{ mODTextTranslationClass(HorizonMgr);
public:
				HorizonMgr(TypeSet<Marker>& hor)
				   : wd_(0)
				   , horizons_(hor)
				   {}

    mStruct(WellAttrib) PosCouple
    {
	float			z1_, z2_;
	bool			operator == ( const PosCouple& pc ) const
				{ return z1_ == pc.z1_ && z2_ == pc.z2_; }
    };

    void			matchHorWithMarkers(TypeSet<PosCouple>&,
							bool bynames) const;
    void			setUpHorizons(const DBKeySet&,
						  uiString&,TaskRunner&);
    void			setWD( const Well::Data* wd)
				{ wd_ = wd; }

protected:

    const Well::Data*		wd_;
    TypeSet<Marker>&		horizons_;
};


mExpClass(WellAttrib) Server : public CallBacker
{ mODTextTranslationClass(Server);
public :
				Server(const WellTie::Setup&);
				~Server();

    const DBKey&		wellID() const	{ return wellid_; }
    Well::Data*			wd()		{ return data_->wd_; }
    const Well::Data*		wd() const	{ return data_->wd_; }

    PickSetMgr&			pickMgr()	{ return *pickmgr_; }
    D2TModelMgr&		d2TModelMgr()	{ return *d2tmgr_; }
    HorizonMgr&			horizonMgr()	{ return *hormgr_; }
    DispParams&			dispParams()	{ return data_->dispparams_; }
    DataWriter&			dataWriter()	{ return *datawriter_; }
    const Data&			data() const	{ return *data_; }

    bool			isOK() const	{ return errmsg_.isEmpty(); }
    const uiString&		errMsg() const	{ return errmsg_; }
    const uiString&		warnMsg() const { return warnmsg_; }

    bool			setNewWavelet(const DBKey&);
    bool			computeSynthetics(const Wavelet&);
    bool			extractSeismics();
    bool			updateSynthetics(const Wavelet&);
    bool			computeAdditionalInfo(const Interval<float>&);
    bool			computeCrossCorrelation();
    bool			computeEstimatedWavelet(int newsz);
    void			setCrossCorrZrg(const Interval<float>&);
    bool			hasSynthetic() const;
    bool			hasSeismic() const;
    bool			doSeismic() const;
    void			updateExtractionRange();

    void			setTaskRunner( TaskRunner* taskrun )
				{ data_->trunner_ = taskrun; }
protected :

    PickSetMgr*			pickmgr_;
    RefMan<Well::Data>		wd_;
    DataPlayer*			dataplayer_;
    HorizonMgr*			hormgr_;
    D2TModelMgr*		d2tmgr_;
    DataWriter*			datawriter_;
    Data*			data_;
    DBKey			wellid_;

    uiString			errmsg_;
    mutable uiString		warnmsg_;

    void			wellDataDel( CallBacker* );
    void			handleDataPlayerWarning() const;
};

} // namespace WellTie
