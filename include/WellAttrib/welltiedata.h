#ifndef welltiedata_h
#define welltiedata_h

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
#include "wellattribmod.h"
#include "callback.h"
#include "color.h"
#include "iopar.h"
#include "multiid.h"
#include "welldisp.h"
#include "welltied2tmodelmanager.h"
#include "welltiesetup.h"

class BinID;
class CtxtIOObj;
class SeisTrc;
class TaskRunner;
class Wavelet;

template <class T> class StepInterval;

namespace Well { class Data; class Log; class LogSet; class Writer; }

namespace WellTie
{
    class DataPlayer;
    class Setup;
    class PickSetMgr;

mExpClass(WellAttrib) DispParams
{
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

    bool                    ismarkerdisp_;
    bool                    isvwrmarkerdisp_;
    bool                    isvwrhordisp_;
    bool                    dispmrkfullnames_;
    bool                    disphorfullnames_;
    bool                    iszinft_;
    bool                    iszintime_;
    Well::DisplayProperties::Markers mrkdisp_;
    BufferStringSet	    allmarkernms_;

    static const char*		sKeyIsMarkerDisp()	{ return
					   "Display Markers on Well Display"; }
    static const char*		sKeyVwrMarkerDisp()	{ return
					      "Display Markers on 2D Viewer"; }
    static const char*		sKeyVwrHorizonDisp()	{ return
					      "Display Horizon on 2D Viewer"; }
    static const char*		sKeyZInFeet()		{ return "Z in Feet"; }
    static const char*		sKeyZInTime()		{ return "Z in Time"; }
    static const char*		sKeyMarkerFullName()	{ return
						 "Display markers full name"; }
    static const char*		sKeyHorizonFullName()	{ return
						 "Display horizon full name"; }	

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&); 
};


mExpClass(WellAttrib) Marker
{
public:
			    Marker(float z)
				: zpos_(z)
				, size_(2)  
				{}

    Color			color_;
    float			zpos_;
    BufferString		name_;
    int 			id_;
    int				size_;

    bool			operator == ( const Marker& m ) const
					{ return m.zpos_ == zpos_; }
};

    
mExpClass(WellAttrib) PickData
{
public:
    TypeSet<Marker>		synthpicks_;
    TypeSet<Marker>		seispicks_;
};

    
mExpClass(WellAttrib) Data
{
public :
    				Data(const Setup&,Well::Data& wd);
    				~Data();

    Well::Data*			wd_;

    Well::LogSet&		logset_;
    SeisTrc&			synthtrc_;
    SeisTrc&			seistrc_;
    Wavelet&			initwvlt_;
    Wavelet&			estimatedwvlt_;
    const Well::Log*		cslog_;
    bool			isinitwvltactive_;

    const StepInterval<float>&	getTraceRange() const	{ return tracerg_; }
    const Interval<float>&	getDahRange() const	{ return dahrg_; }
    const StepInterval<float>&	getModelRange() const	{ return modelrg_; }
    const StepInterval<float>&	getReflRange() const	{ return reflrg_; }

    const Setup&		setup() const	{ return setup_; }
    const char* 		density() const	{ return setup_.denlognm_; }
    const char* 		sonic() const	{ return setup_.vellognm_; }
    bool			isSonic() const	{ return setup_.issonic_; }

    static const char* 		ai()		{ return "AI"; }
    static const char* 		reflectivity()	{ return "Reflectivity"; }
    static const char* 		synthetic()	{ return "Synthetic"; }
    static const char* 		seismic()	{ return "Seismic"; }
    static float		cDefSeisSr();

    TypeSet<Marker>		horizons_;
    PickData			pickdata_;
    DispParams			dispparams_;
    TaskRunner*			trunner_;

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
    const Setup&		setup_;
};


mExpClass(WellAttrib) WellDataMgr : public CallBacker
{
public:
    				WellDataMgr(const MultiID&);
    				~WellDataMgr();

    Well::Data*          	wd() 		{ return wellData(); }
    Notifier<WellDataMgr>	datadeleted_;

protected:

    Well::Data*          	wellData() const;

    Well::Data*          	wd_;
    const MultiID		wellid_;
    void			wellDataDelNotify(CallBacker*);
};


mExpClass(WellAttrib) DataWriter 
{	
public:    
				DataWriter(Well::Data&,const MultiID&);
				~DataWriter();

    mStruct(WellAttrib) LogData
    {
				LogData( const Well::LogSet& logset )
				    : logset_(logset)
				    , curlog_(0)  
				    {}

	ObjectSet<CtxtIOObj> 	seisctioset_;    
	const Well::LogSet& 	logset_;			    
	const Well::Log*	curlog_;	 
	TypeSet<BinID> 		bids_;
	TypeSet<int>		ctioidxset_;	 
	int			nrtraces_;
	int 			curidx_;
    };			    

    bool 			writeD2TM() const;		
    bool                        writeLogs(const Well::LogSet&) const;
    bool                        writeLogs2Cube(LogData&,Interval<float>) const;

    void			setWD(Well::Data* wd)
    				{ wd_ = wd; setWellWriter(); }

    const char*			errMsg() const 
    				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

protected:

    Well::Writer* 		wtr_;
    Well::Data*			wd_;
    const MultiID&		wellid_;
    BufferString		errmsg_;

    void 			setWellWriter();
    bool                        writeLog2Cube(LogData&) const;
};


mExpClass(WellAttrib) HorizonMgr
{
public:
    				HorizonMgr(TypeSet<Marker>& hor)
				   : wd_(0)
				   , horizons_(hor)  
				   {}

    mStruct(WellAttrib) PosCouple 		
    { 
	float 			z1_, z2_; 
	bool 			operator == ( const PosCouple& pc ) const
				{ return z1_ == pc.z1_ && z2_ == pc.z2_; }
    };

    void			matchHorWithMarkers(TypeSet<PosCouple>&,
	    						bool bynames) const;
    void			setUpHorizons(const TypeSet<MultiID>&,
						  BufferString&,TaskRunner&);
    void			setWD( const Well::Data* wd)
				{ wd_ = wd; }

protected:

    const Well::Data*		wd_;
    TypeSet<Marker>&		horizons_;
};


mExpClass(WellAttrib) Server : public CallBacker
{
public :
    				Server(const WellTie::Setup&);
    				~Server();

    const Well::Data* 		wd() const	{ return data_->wd_; }
    Well::Data* 		wd()		{ return data_->wd_; }

    const MultiID&		wellID() const	{ return wellid_; }

    PickSetMgr&			pickMgr() 	{ return *pickmgr_; }
    D2TModelMgr&		d2TModelMgr()	{ return *d2tmgr_; }
    HorizonMgr&			horizonMgr() 	{ return *hormgr_; }
    DispParams&			dispParams()	{ return data_->dispparams_; }
    DataWriter&			dataWriter()	{ return *datawriter_; } 
    const Data&			data() const 	{ return *data_; }

    const char* 		errMSG() const	
    				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

    bool			is2D() const	{ return is2d_; }

    bool			computeSynthetics();
    bool			extractSeismics();
    bool			updateSynthetics();
    bool			computeAdditionalInfo(const Interval<float>&);
    bool			hasSynthetic() const;
    bool			hasSeismic() const;
    bool			doSeismic() const;

    void			setInitWvltActive(bool yn)
				{ data_->isinitwvltactive_ = yn; }
    void			setTaskRunner( TaskRunner* tr )
				{ data_->trunner_ = tr; }
protected :
    PickSetMgr*			pickmgr_;
    WellDataMgr*		wdmgr_;
    DataPlayer*			dataplayer_;
    HorizonMgr*			hormgr_;
    D2TModelMgr*		d2tmgr_;
    DataWriter*			datawriter_;
    Data* 			data_;
    const MultiID&		wellid_;

    bool			is2d_;
    BufferString		errmsg_;

    void			wellDataDel( CallBacker* );
};

}; //namespace WellTie


#endif


