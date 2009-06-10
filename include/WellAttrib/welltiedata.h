#ifndef welltiedata_h
#define welltiedata_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
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
class WellTieD2TModelMGR;   
class WellTieDataSet;   
class WellTiePickSetMGR;   
class DataPointSet;
namespace Well
{
    class Data;
}

// brief structure containing datasets and data used for TWTS
mStruct WellTieData
{
			WellTieData()
			    : nrdataset_(3) 
			    {}

    int 		nrdataset_;
    float		corrcoeff_;
    Wavelet		wvltest_;
    
    ObjectSet<WellTieDataSet> datasets_;
};


mClass WellTieDataSet
{
public:
			WellTieDataSet(){};
			~WellTieDataSet(){};

			WellTieDataSet( const WellTieDataSet& ds )
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

mClass WellTieDataSetMGR
{
public:
			WellTieDataSetMGR(const WellTieParams*,WellTieData*);
			~WellTieDataSetMGR();

    void 		resetData();
    void 		resetData(WellTieDataSet&,int);
    void		clearData();
    void 		setWork2DispData();
    void                rescaleData(const WellTieDataSet&,WellTieDataSet&,
	    				int,int);
    void                rescaleData(const WellTieDataSet&,WellTieDataSet&,
	    				int,float,float);
    void 		getSortedDPSDataAlongZ( const DataPointSet&,
	   				        Array1DImpl<float>& );
    
protected:

    const WellTieParams& params_;
    ObjectSet<WellTieDataSet>& datasets_;
};


//brief contains all the data, params and mgrs needed by TWTS
mClass WellTieDataHolder 
{
public:    
			WellTieDataHolder(const WellTieParams*,Well::Data*,
					  const WellTieSetup&);
			~WellTieDataHolder();

    const WellTieParams*  params() const   { return params_; }   
    const WellTieSetup&	  setup()  const   { return setup_; }
    WellTieData&	  data()   	   { return data_; }
    const WellTieData&	  data()   const   { return data_; }
    Well::Data* 	  wd()        	   { return wd_; }	
    const Well::Data* 	  wd()     const   { return wd_; }	
    WellTieD2TModelMGR*   d2TMGR()	   { return d2tmgr_; }   
    const WellTieD2TModelMGR* d2TMGR() const { return d2tmgr_; }   
    WellTieDataSet*	  extrData() 	   { return data_.datasets_[0]; }
    const WellTieDataSet* extrData() const { return data_.datasets_[0]; }
    WellTieDataSet*	  dispData() 	   { return data_.datasets_[1]; }
    const WellTieDataSet* dispData() const { return data_.datasets_[1]; }
    WellTieDataSet*	  corrData()  	   { return data_.datasets_[2]; } 
    const WellTieDataSet* corrData() const { return data_.datasets_[2]; } 
    
    WellTieD2TModelMGR*	 d2tmgr_;
    WellTiePickSetMGR*   pickmgr_;
    WellTieDataSetMGR*	 datamgr_;

private:

    Well::Data*          wd_;
    const WellTieSetup&	 setup_;
    WellTieData	 	 data_;

    const WellTieParams* params_;
};
#endif
