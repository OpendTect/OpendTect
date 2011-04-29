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

#include "callback.h"
#include "color.h"
#include "multiid.h"
#include "welltied2tmodelmanager.h"

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

mStruct DispParams
{
			    DispParams()
			    : iscsdisp_(false)
			    , ismarkerdisp_(true)
			    , isvwrmarkerdisp_(true)
			    , isvwrhordisp_(false)
			    , iszinft_(false)
			    , iszintime_(true)
			    , dispmrkfullnames_(false)
			    , disphorfullnames_(false)
			    {}

    bool                    iscsavailable_;
    bool                    iscscorr_;
    bool                    iscsdisp_;
    bool                    ismarkerdisp_;
    bool                    isvwrmarkerdisp_;
    bool                    isvwrhordisp_;
    bool                    dispmrkfullnames_;
    bool                    disphorfullnames_;
    bool                    iszinft_;
    bool                    iszintime_;
};


mStruct Marker
{
			    Marker(float z)
				: zpos_(z)
				{}

    Color			color_;
    float			zpos_;
    const char*			name_;
    int 			id_;

    bool			operator == ( const Marker& m ) const
					{ return m.zpos_ == zpos_; }
};

mStruct PickData		{ TypeSet<Marker> synthpicks_, seispicks_; };

mClass Data
{
public :
    				Data(const Setup&);
    				~Data();

    Well::Data*			wd_;

    Well::LogSet&		logset_;
    SeisTrc&			synthtrc_;
    SeisTrc&			seistrc_;
    Wavelet&			initwvlt_;
    Wavelet&			estimatedwvlt_;
    bool			isinitwvltactive_;
    const StepInterval<float>& 	timeintv_;
    const Setup&		setup() const	{ return setup_; }

    const char*  		sonic() 	const;
    const char*  		corrsonic() 	const;
    const char*  		currvellog() 	const;
    const char*  		checkshotlog() 	const;
    const char*  		density() 	const;
    const char*  		ai() 		const;
    const char*  		reflectivity() 	const;
    const char*  		synthetic() 	const;
    const char*  		seismic() 	const;
    bool			isSonic() 	const;

    void			setVelLogName( bool iscs )
				{ currvellog_ = iscs ? corrsonic() : sonic(); }
    TypeSet<Marker>		horizons_;
    PickData			pickdata_;
    DispParams			dispparams_;

protected:

    const Setup&		setup_;
    const char* 		currvellog_;
};


mClass WellDataMgr : public CallBacker
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


mClass DataWriter 
{	
public:    
				DataWriter(Well::Data*,const MultiID&);
				~DataWriter();

    mStruct LogData
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
    bool                        writeLogs2Cube(LogData&) const;

protected:

    Well::Writer* 		wtr_;
    Well::Data*			wd_;
    const MultiID&		wellid_;

    void 			setWellWriter();
    bool                        writeLog2Cube(LogData&) const;
};


mClass HorizonMgr
{
public:
    				HorizonMgr(TypeSet<Marker>& hor)
				   : wd_(0)
				   , horizons_(hor)  
				   {}

    mStruct PosCouple 		
    { 
	float z1_, z2_; 
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


mClass Server : public CallBacker
{
public :
    				Server(const WellTie::Setup&);
    				~Server();

    PickSetMgr&			pickMgr() 	{ return *pickmgr_; }
    HorizonMgr&			horizonMgr() 	{ return *hormgr_; }
    DispParams&			dispParams()	{ return data_.dispparams_; }

    const char* 		errMSG() const	{ return errmsg_.buf(); }

    const Well::Data* 		wd() const	{ return data_.wd_; }
    const Data&			data() const 	{ return data_; }

    bool			is2D() const	{ return is2d_; }

    void			setVelLogName( bool iscs )
				{ data_.setVelLogName( iscs ); }

    void			resetD2TModel( Well::D2TModel* d2t )
				{ d2tmgr_->setAsCurrent(d2t); }
    bool                	undoD2TModel()
				{ return d2tmgr_->undo(); }
    bool                	cancelD2TModel()
    				{ return d2tmgr_->cancel(); }
    bool                	commitD2TModel()
				{ return d2tmgr_->commitToWD(); }
    void			replaceTime(const Array1DImpl<float>& tarr)
				{ d2tmgr_->replaceTime( tarr ); }

    bool			computeAll();
    bool			computeSynthetics();

    void			setEstimatedWvlt(float*,int);
    void			setInitWvltActive(bool yn)
				{ data_.isinitwvltactive_ = yn; }
protected :
    PickSetMgr*			pickmgr_;
    WellDataMgr*		wdmgr_;
    DataPlayer*			dataplayer_;
    HorizonMgr*			hormgr_;
    D2TModelMgr*		d2tmgr_;
    Data 			data_;

    bool			is2d_;
    BufferString		errmsg_;

    void			wellDataDel( CallBacker* );
};

}; //namespace WellTie


#endif
