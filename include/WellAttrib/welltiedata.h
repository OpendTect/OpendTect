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
#include "bufstringset.h"


template <class T> class Array1DImpl;
class WellTieParams;
class DataPointSet;

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
    Array1DImpl<float>* get( int idx )        	    { return data_[idx]; }
    const Array1DImpl<float>* get( int idx ) const  { return data_[idx]; }
    Array1DImpl<float>* get( const char* colname )
    					{ return data_[getColIdx(colname)]; }

    const int           getLength() const           { return datasz_; }
    void                setLength( int datasz )     { datasz_ = datasz; }
    void                setColNr( int colnr)        { colnr_ = colnr; }
    void                setColNames( BufferStringSet nms) { colnameset_ = nms; }
    void                clearData();
    void                createDataArrays();
    const float 	getExtremVal(const char*, bool);
    const int           getIdx(float);
    const float         get(const char*,int);
    const float         get(int,int);
    const int           getColIdx(const char*);
    const int 		findTimeIdx(float);


protected:

    int                 colnr_;
    int                 datasz_;

    BufferStringSet     colnameset_;
    ObjectSet< Array1DImpl<float> > data_;
};



/*!\brief Manages the data used during TWTS. */

mClass WellTieDataMGR
{
public:
			WellTieDataMGR(const WellTieParams*);
			~WellTieDataMGR();

    WellTieDataSet*	getDispData() 		{ return &dispdata_; }	
    WellTieDataSet*	getWorkData()		{ return &workdata_; }

    void 		resetData();
    void 		resetData(WellTieDataSet&,int);
    void		clearData();
    void 		setWork2DispData();
    void                rescaleData(const WellTieDataSet&,WellTieDataSet&,
	    				int,int);
    void 		getSortedDPSDataAlongZ( const DataPointSet&,
	   				        Array1DImpl<float>& );
    
protected:

    const WellTieParams& params_;
    WellTieDataSet	dispdata_;
    WellTieDataSet	workdata_;
};


#endif
