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

#include "namedobj.h"
#include "arrayndimpl.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltieunitfactors.h"

class DataPointSet;
class Wavelet;
namespace Well { class Data; }

namespace WellTie
{
    class D2TModelMGR;   
    class DataHolder;
    class GeoCalculator;   
    class PickSetMGR;  
    class Setup;

mClass Log : public Well::Log
{
public :
    			Log(const char*);
			~Log();


    const Array1DImpl<float>* getVal(const Interval<float>*,bool);
    void		setVal(const Array1DImpl<float>*,bool);

protected:

    Array1DImpl<float>* arr_;
};


/*!\brief Manages the datasets used during TWTS. */
#define mDynCast(nm,act)\
    mDynamicCastGet(WellTie::Log*,l,gtLog(nm)); if (!l) act;
mClass LogSet : public Well::LogSet
{
public:
			LogSet(WellTie::DataHolder&){};
			~LogSet();

    void 		resetData(const WellTie::Params::DataParams&);
    const Array1DImpl<float>* getVal(const char*,bool isdah=false,
	    			     const Interval<float>* st=0) const;
    void		setVal(const char* nm,const Array1DImpl<float>* val,
			       bool isdah = false)
			{ mDynCast(nm,return); l->setVal(val,isdah); }
    float		get(const char* nm,int idx) const
			{ return (getVal(nm)) ? getVal(nm)->get(idx):0; }
    float 		getExtremVal(const char*,bool) const;
};


//brief contains all the data, params and mgrs needed by TWTS
mClass DataHolder 
{
public:    
			DataHolder(WellTie::Params*,Well::Data*,
				  const WellTie::Setup&);
			~DataHolder();

//WellData			
    Well::Data* 	  wd()        	   { return wd_; }	
    const Well::Data* 	  wd()     const   { return wd_; }	

//logs 
    WellTie::LogSet*  	  logsset() 	{ return logsset_; }
    const WellTie::LogSet* logsset() const { return logsset_; }

//Wavelet
    ObjectSet<Wavelet>&		wvltset() { return wvltset_; }
    const ObjectSet<Wavelet>&	wvltset() const { return wvltset_; }

//Params
    const WellTie::Setup& setup()  const   { return setup_; }
    const WellTie::Params*  	params() const   { return params_; }   
    WellTie::Params::uiParams* 	uipms()    { return &params_->uipms_;  }
    const WellTie::Params::uiParams* uipms() const { return &params_->uipms_;  }
    WellTie::Params::DataParams* dpms()    { return &params_->dpms_; }
    const WellTie::Params::DataParams* dpms() const { return &params_->dpms_; }
    const WellTie::UnitFactors& getUnits() const   { return factors_; }

//MGRs
    WellTie::D2TModelMGR* d2TMGR()	   { return d2tmgr_; }   
    const WellTie::D2TModelMGR* d2TMGR() const { return d2tmgr_; }   
    WellTie::PickSetMGR*  pickmgr()   	   { return pickmgr_; }
    const WellTie::PickSetMGR* pickmgr() const { return pickmgr_; }

//Others    
    float&		corrcoeff() 	   { return corrcoeff_; }
    const float&	corrcoeff() const  { return corrcoeff_; }
    WellTie::GeoCalculator* geoCalc()	   { return geocalc_; } 
    const WellTie::GeoCalculator* geoCalc() const { return geocalc_; } 
    
private:

    float 			corrcoeff_;
    Well::Data*          	wd_;

    WellTie::UnitFactors	factors_;
    WellTie::LogSet*		logsset_;
    WellTie::D2TModelMGR*	d2tmgr_;
    WellTie::Params* 	 	params_; //becomes mine
    WellTie::Params::uiParams* 	uipms_;
    WellTie::Params::DataParams* dpms_;
    WellTie::PickSetMGR*   	pickmgr_;
    WellTie::GeoCalculator* 	geocalc_;

    const WellTie::Setup&	setup_;
    ObjectSet<Wavelet>		wvltset_;
};

}; //namespace WellTie
#endif
