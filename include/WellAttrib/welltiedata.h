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
#include "arrayndimpl.h"
#include "welltieunitfactors.h"

class DataPointSet;
class BinID;
class Wavelet;
class CtxtIOObj;
class Color;
class TaskRunner;
namespace Well { class Data; class Log; class LogSet; class Writer; }

namespace WellTie
{
    class D2TModelMGR;   
    class DataHolder;
    class GeoCalculator;   
    class PickSetMGR;  
    class Setup;


//brief contains all the data, params and mgrs needed by TWTS
mClass DataHolder : public CallBacker 
{
public:    
				DataHolder(const WellTie::Setup&);
				~DataHolder();

//WellData			
    Well::Data* 		wd() const;
    const BinID			binID() const;    
    const MultiID& 		wellid_;
    Notifier<DataHolder>	closeall;
    Notifier<DataHolder>	redrawViewerNeeded;
    void			welldataDelNotify(CallBacker*);
    //void			triggerClose();

//logs 
    Well::LogSet*  	  	logset() 	{ return logset_; }
    const Well::LogSet* 	logset() const 	{ return logset_; }
    Array1DImpl<float>*		arr(const char* nm) { return getLogVal(nm); }
    Array1DImpl<float>* 	getLogVal(const char*,bool dah=false) const;
    void 			setLogVal(const char*,const Array1DImpl<float>*,
    					  const Array1DImpl<float>*);
    void 			resetLogData();
   
//Wavelet
    ObjectSet<Wavelet>&		wvltset() 	{ return wvltset_; }
    const ObjectSet<Wavelet>&	wvltset() const { return wvltset_; }

//Params
    const WellTie::Setup& setup()  const   	{ return setup_; }
    WellTie::Params*  		params()  	{ return params_; }   
    const WellTie::Params*  	params() const  { return params_; }   
    WellTie::Params::uiParams* 	uipms()    	{ return &params_->uipms_;  }
    const WellTie::Params::uiParams* uipms() const { return &params_->uipms_;  }
    WellTie::Params::DataParams* dpms()    	{ return &params_->dpms_; }
    const WellTie::Params::DataParams* dpms() const { return &params_->dpms_; }
    const WellTie::UnitFactors& getUnits() const { return factors_; }

//MGRs
    WellTie::D2TModelMGR* d2TMGR()	   	{ return d2tmgr_; }   
    const WellTie::D2TModelMGR* d2TMGR() const 	{ return d2tmgr_; }   
    WellTie::PickSetMGR*  pickmgr()   	   	{ return pickmgr_; }
    const WellTie::PickSetMGR* pickmgr() const 	{ return pickmgr_; }

//CtxtIobj
    CtxtIOObj*			seisCtxt() 	{ return &seisctio_; }
    const CtxtIOObj*		seisCtxt() const { return &seisctio_; }
    CtxtIOObj*			wvltCtxt() 	{ return &wvltctio_; }
    const CtxtIOObj*		wvltCtxt() const { return &wvltctio_; }

//Horizons
    mStruct HorData
    {
				HorData(float z,const Color& col)
				    : zval_(z)
				    , color_(col)
				    {}

	float			zval_;
	const Color&		color_;
	BufferString		name_;
	int 			lvlid_;	
    };
    const ObjectSet<HorData>	horDatas() const { return hordatas_; }
    bool			matchHorWithMarkers(BufferString&,bool bynames);
    bool			setUpHorizons(const TypeSet<MultiID>&,
						BufferString&,TaskRunner&);

//Others    
    float&			corrcoeff() 	{ return corrcoeff_; }
    const float&		corrcoeff() const { return corrcoeff_; }
    WellTie::GeoCalculator* 	geoCalc() 	{ return geocalc_; } 
    const WellTie::GeoCalculator* geoCalc() const { return geocalc_; } 
    
private:

    float 			corrcoeff_;
    ObjectSet<HorData>		hordatas_;
    Well::Data*          	wd_;

    CtxtIOObj&                  seisctio_;
    CtxtIOObj&                  wvltctio_;

    WellTie::UnitFactors	factors_;
    WellTie::D2TModelMGR*	d2tmgr_;
    WellTie::Params* 	 	params_; //becomes mine
    WellTie::Params::uiParams* 	uipms_;
    WellTie::Params::DataParams* dpms_;
    WellTie::PickSetMGR*   	pickmgr_;
    WellTie::GeoCalculator* 	geocalc_;

    const WellTie::Setup&	setup_;
    Well::LogSet*		logset_;
    Array1DImpl<float>*		arr(int idx) { return arr_[idx]; }
    ObjectSet<Wavelet>		wvltset_;
    ObjectSet< Array1DImpl<float> > arr_;
};


mClass DataWriter 
{	
public:    
				DataWriter(const WellTie::DataHolder&);
				~DataWriter();
   
    mStruct LogData
    {
	public:
				LogData( const Well::LogSet& logset )
				: logset_(logset)
				{}

	 ObjectSet<CtxtIOObj> 	seisctioset_;    
	 const Well::LogSet& 	logset_;			    
	 TypeSet<BinID> 	bids_;
	 TypeSet<int>		ctioidxset_;	 
	 int			nrtraces_;
	 int 			curidx_;

	 const Well::Log*	curlog_;	 
    };			    

    bool 			writeD2TM() const;		
    bool                        writeLogs(const Well::LogSet&) const;
    bool                        writeLogs2Cube(LogData&) const;

protected:

    const WellTie::DataHolder&	holder_;
    Well::Writer* 		wtr_;

    void 			setWellWriter();
    bool                        writeLog2Cube(LogData&) const;
};

}; //namespace WellTie
#endif
