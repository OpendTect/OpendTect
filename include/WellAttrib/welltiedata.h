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
#include "wavelet.h"
#include "bufstringset.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"


template <class T> class Array1DImpl;
class DataPointSet;
namespace Well
{
    class Data;
    class Log;
}

namespace WellTie
{
    class D2TModelMGR;   
    class DataSet;   
    class PickSetMGR;   

// brief structure containing datasets and data used for TWTS
mStruct Data
{
			Data()
			: nrdataset_(3) 
			{}

    int 		nrdataset_;
    float		corrcoeff_;
    Wavelet		wvltest_;
    
    ObjectSet<WellTie::DataSet> datasets_;
    ObjectSet<Well::Log> logset_;
};


mClass DataSet
{
public:
			DataSet(){};
			~DataSet(){};

			DataSet( const DataSet& ds )
			    : colnr_(ds.colnr_)
			    , datasz_(ds.datasz_)
			    , colnameset_(ds.colnameset_)
			    {
				deepCopy(data_,ds.data_);
			    }

    ObjectSet< Array1DImpl<float> >& getSet()       { return data_; }
    Array1DImpl<float>* get(int idx)        	    { return data_[idx]; }
    const Array1DImpl<float>* get(int idx) const  { return data_[idx]; }
    Array1DImpl<float>* get(const char* colname)
				    { return data_[getColIdx(colname)]; }
    const Array1DImpl<float>* get(const char* colname) const
				    { return data_[getColIdx(colname)]; }

    const bool          isEmpty() const		{ return data_.isEmpty(); }
    const int           getLength() const           { return datasz_; }
    void                setLength( int datasz )     { datasz_ = datasz; }
    void                setColNr( int colnr)        { colnr_ = colnr; }
    void                setColNames( BufferStringSet nms ) 
    						    { colnameset_ = nms; }
    void                clearData();
    void                createDataArrays();
    const float 	getExtremVal(const char*, bool) const;
    void 		setArrayBetweenIdxs(const Array1DImpl<float>&,
				Array1DImpl<float>&,int,int);
    const int           getIdx(float time) const;
    const int           getIdxFromDah(float) const;
    const float         get(const char*,int) const;
    const float         get(int,int) const;
    const int           getColIdx(const char*) const;
    void         	set(const char*,int,float);

protected:

    int                 colnr_;
    int                 datasz_;

    BufferStringSet     colnameset_;
    ObjectSet< Array1DImpl<float> > data_;
};



/*!\brief Manages the datasets used during TWTS. */

mClass DataSetMGR
{
public:
			DataSetMGR(const WellTie::Params::DataParams*,
				    WellTie::Data*);
			~DataSetMGR();

    void 		resetData();
    void 		resetData(WellTie::DataSet&,int);
    void		clearData();
    void 		setWork2DispData();
    void                rescaleData(const WellTie::DataSet&,WellTie::DataSet&,
	    				int,int);
    void                rescaleData(const WellTie::DataSet&,WellTie::DataSet&,
	    				int,float,float);
    void 		getSortedDPSDataAlongZ( const DataPointSet&,
	   				        Array1DImpl<float>& );
    
protected:

    const WellTie::Params::DataParams& params_;
    ObjectSet<WellTie::DataSet>& datasets_;
};


//brief contains all the data, params and mgrs needed by TWTS
mClass DataHolder 
{
public:    
			DataHolder(WellTie::Params*,Well::Data*,
				  const WellTie::Setup&);
			~DataHolder();

    const WellTie::Params*  	params() const   { return params_; }   
    WellTie::Params::uiParams* 	uipms()       { return &params_->uipms_;  }
    const WellTie::Params::uiParams* uipms() const { return &params_->uipms_;  }
    WellTie::Params::DataParams* dpms() 	    { return &params_->dpms_; }
    const WellTie::Params::DataParams* dpms() const { return &params_->dpms_; }
    const WellTie::Setup& setup()  const   { return setup_; }
    WellTie::Data&	  data()   	   { return data_; }
    const WellTie::Data&  data()   const   { return data_; }
    Well::Data* 	  wd()        	   { return wd_; }	
    const Well::Data* 	  wd()     const   { return wd_; }	
    WellTie::D2TModelMGR* d2TMGR()	   { return d2tmgr_; }   
    const WellTie::D2TModelMGR* d2TMGR() const { return d2tmgr_; }   
    WellTie::PickSetMGR*  pickmgr()   	   { return pickmgr_; }
    const WellTie::PickSetMGR* pickmgr() const { return pickmgr_; }
    WellTie::DataSetMGR*  datamgr()	   { return datamgr_; }
    const WellTie::DataSetMGR* 	datamgr() const { return datamgr_; }
    WellTie::DataSet* 	  extrData() 	   { return data_.datasets_[0]; }
    const WellTie::DataSet* extrData() const { return data_.datasets_[0]; }
    WellTie::DataSet*	  dispData() 	   { return data_.datasets_[1]; }
    const WellTie::DataSet* dispData() const { return data_.datasets_[1]; }
    WellTie::DataSet*	  corrData()  	   { return data_.datasets_[2]; } 
    const WellTie::DataSet* corrData() const { return data_.datasets_[2]; } 
    Wavelet*              getEstimatedWvlt()  { return &data_.wvltest_; } 
    const Wavelet*        getEstimatedWvlt() const { return &data_.wvltest_; } 

    
private:

    Well::Data*          	wd_;

    WellTie::Data	 	data_;
    WellTie::DataSetMGR*	datamgr_;
    WellTie::D2TModelMGR*	d2tmgr_;
    WellTie::Params::uiParams* 	uipms_;
    WellTie::Params* 	 	params_; //becomes mine
    WellTie::Params::DataParams* dpms_;
    WellTie::PickSetMGR*   	pickmgr_;
    const WellTie::Setup&	setup_;

};

}; //namespace WellTie
#endif
