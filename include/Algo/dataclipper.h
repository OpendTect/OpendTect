#ifndef dataclipper_h
#define dataclipper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		09-02-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "sets.h"
#include "ranges.h"
class IOPar;
template <class T> class ValueSeries;
template <class T> class ArrayND;

/*!
\ingroup Algo
\brief A DataClipper gets a bunch of data and determines at what value to
clip if a certain clip percentage is desired.
  
  For simple cases, where no subselection is needed (i.e. the stats will be
  performed on all values, and only one dataset is used) the static function
  calculateRange is good enough:
  
  \code
  TypeSet<float> mydata;
  Interval<float> range;
  DataClipper::calculateRange( mydata.arr(), mydata.size(), 0.05, 0.05,
  range );
  \endcode
  
  If there are more than one dataset, or if a subselection is wanted, the class
  is used as follows:
  
  -# Create object
  -# If subselection is wanted, set total nr of samples and statsize with
  setApproxNrValues
  -# Add all your sources putData
  -# If you only want a fixed range, call calculateRange.
  -# If you want to come back an get multiple ranges, call fullSort. After
  fullSort, the getRange functions can be called, any number of times.
  -# To prepare the object for a new set of data, call reset.
  
  Example
  \code
  Array3D<float> somedata;
  TypeSet<float> moredata;
  float	   otherdata;
  
  DataClipper clipper;
  setApproxNrValues( somedata.info().getTotalSz()+moredata.size()+1, 2000 );
  clipper.putData( somedata );
  clipper.putData( moredata.arr(), moredata.size() );
  clipper.putData( otherdata );
  
  clipper.fullSort();
  
  Interval<float> clip99;
  Interval<float> clip95;
  Interval<float> clip90;
  clipper.getRange( 0.01, clip99 );
  clipper.getRange( 0.05, clip95 );
  clipper.getRange( 0.10, clip90 );
  \endcode
  
*/

mExpClass(Algo) DataClipper
{
public:
    				DataClipper();
				/*!< cliprate is between 0 and 0.5,
				     cliprate0 is the bottom cliprate,
				     cliprate1 is the top cliprate, when
				     cliprate1 is -1, it will get the value of
				     cliprate0 */
    inline bool			isEmpty() const	{ return samples_.isEmpty(); }

    void			setApproxNrValues(od_int64 nrsamples,
						  int statsize=2000);
    				/*!< Will make it faster if large amount
				     of data is used. The Object will then
				     randomly subselect on the input to get
				     about statsize samples to do the stats on.
				*/
    void			putData(float);
    void			putData(const float*,od_int64 sz);
    void			putData(const ValueSeries<float>&,od_int64 sz);
    void			putData(const ArrayND<float>&);

    bool			calculateRange(float cliprate,Interval<float>&);
    				/*!<Does not do a full sort. Also performes
				    reset */
    bool			calculateRange(float lowcliprate,
	    				       float highcliprate,
					       Interval<float>&);
    				/*!<Does not do a full sort. Also performes
				    reset */
    static bool			calculateRange(float* vals, od_int64 nrvals,
	    				       float lowcliprate,
	    				       float highcliprate,
					       Interval<float>&);
    				/*!<Does not do a full sort.\note vals
				    are modified. */
    bool			fullSort();
    bool			getRange(float cliprate,Interval<float>&) const;
    bool			getRange(float lowcliprate,float highcliprate,
	    				 Interval<float>&) const;
    bool			getSymmetricRange(float cliprate,float midval,
	    				 Interval<float>&) const;
    void			reset();

    const LargeValVec<float>&	statPts() const { return samples_; }

protected:

    int				approxstatsize_;
    float			sampleprob_;
    bool			subselect_;
    LargeValVec<float>		samples_;
    Interval<float>		absoluterg_;
};


/*!
\ingroup Algo
\brief Data clipping sampler.
*/

mExpClass(Algo) DataClipSampler
{
public:
			DataClipSampler(int bufsz=10000);
			~DataClipSampler() 	{ delete [] vals_; }

    void		reset()			{ count_ = 0; }

    void		add(float);
    void		add(const float*,od_int64);
    void		finish() const;

    od_int64		nrVals() const;
    const float*	vals() const		{ return vals_; }

    void		report(IOPar&) const;
    Interval<float>	getRange(float clipratio) const;


protected:

    float*		vals_;
    const int		maxnrvals_;
    od_int64		count_;
    bool		finished_;

    void		doAdd(float);
    const char*		getClipRgStr(float) const;
};


#endif

