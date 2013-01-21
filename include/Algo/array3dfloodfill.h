#ifndef array3dfloodfill_h
#define array3dfloodfill_h


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K.Tingdahl/Y.C.Liu
 Date:          Nov 2008
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndinfo.h"
#include "arrayndimpl.h"
#include "multidimstorage.h"
#include "task.h"
#include "thread.h"
#include "varlenarray.h"


#define mMaxNrComp	10


/*!
\brief Given an input array and a threshold, we use flood fill to find all the
locations with values less (or greater) than the threshold based on seeds.
User has the option to set inside or outside value on the output. 
    
    Example: Given known array, threshold, T=float

   	     Array3DImpl<float> output( array.info() ); 
	     Array3DFloodfill<float> floodfill( arr, threshold, max, output );
	     floodfill.setOutsideValue( 1e+5 );
	     
	     floodfill.addSeed(0,0,0);  
	     //At least one seed's value should be bigger than the threshold
	     //if max (or smaller if !max) 

	     floodfill.execute();
*/

template <class T>
class Array3DFloodfill : public ParallelTask
{
public:
    			Array3DFloodfill(const Array3D<T>& input,T threshold,
					 bool aboveisovalue,Array3D<T>& output);
			~Array3DFloodfill();

    void		setOutsideValue(T val);
    			/*!<If udf, uDf(T) will be set. Must be set if use 
			    Marchingcubes. */
    void		setInsideValue(T val)	{ insideval_ = val; }
    			/*!<If udf, input value will be used. */
    void		useInputValue(bool yn)	{ useinputval_ = yn; }
    void		use6Neighbors(bool yn)	{ use6neighbors_ = yn; }
    			/*<The true, we use 6 neighbors, otherwise, use 26. */

    void		addSeed(int,int,int);
    bool		isAboveIsovalue() const { return aboveisovalue_; }
    int			maxNrThreads() const 	{ return compartments_.size(); }
    od_int64		nrIterations() const{return input_.info().getTotalSz();}
   

protected:

    void			setOutput(int,int,int,bool addseed);
    bool			doWork(od_int64 start,od_int64 stop,int);
    int				getNextWorkCompartment();
    void			returnCompartment(int);

    int				getWorkCompartment(int,int,int) const;

    T				threshold_;
    bool			aboveisovalue_;
    int				use6neighbors_;
    bool			useinputval_;
    T				insideval_;
    T				outsideval_;

    int				nrcomp0_;
    int				compsz0_;
    int				nrcomp1_;
    int				compsz1_;
    int				nrcomp2_;
    int				compsz2_;

    int				sz0_;
    int				sz1_;
    int				sz2_;

    struct Compartment
    {				
				Compartment() : isused_( false )
					      , seeds_( 3, 0 ) {}
	bool			isused_;//!<Protected by compartmentlock_
	Threads::ReadWriteLock	lock_;
	MultiDimStorage<int>	seeds_;	//!<Protected by lock_
    };

    ObjectSet<Compartment>	compartments_;
    Threads::ConditionVar	compartmentlock_;
    				//!<Protects the isused_ flags on the
				//!<compartments. 
    TypeSet<int>		permutation_;

    const Array3D<T>&		input_;
    				//!<Not protected
    Array3D<T>&			output_;
    				/*!<The locks_ on the compartment protects 'its'
				    part of the array. */
    Array3DImpl<bool>*		isdefined_;
    				/*!<The locks_ on the compartment protects 'its'
				    part of the array. */

};


template <class T> inline
Array3DFloodfill<T>::Array3DFloodfill( const Array3D<T>& input, T threshold, 
				       bool max, Array3D<T>& output )
    : input_( input )
    , output_( output )
    , threshold_( threshold )
    , aboveisovalue_( max )
    , insideval_( mUdf(T) )				     
    , use6neighbors_( true )
    , useinputval_( true )			    
    , sz0_( input.info().getSize(0) )
    , sz1_( input.info().getSize(1) )
    , sz2_( input.info().getSize(2) )
    , isdefined_( 0 )
    , nrcomp0_( 0 )		     
    , nrcomp1_( 0 )		     
    , nrcomp2_( 0 )
    , compsz0_( 0 )	     
    , compsz1_( 0 )	     
    , compsz2_( 0 )	     
{
    isdefined_ = new Array3DImpl<bool>( sz0_, sz1_, sz2_ );
    memset( isdefined_->getData(), 0, sizeof(bool)*sz0_*sz1_*sz2_ );

    setOutsideValue( mUdf(T) );

    compsz0_ = sz0_/mMaxNrComp; if ( compsz0_<3 ) compsz0_ = 3;
    compsz1_ = sz1_/mMaxNrComp; if ( compsz1_<3 ) compsz1_ = 3;
    compsz2_ = sz2_/mMaxNrComp; if ( compsz2_<3 ) compsz2_ = 3;
    
    nrcomp0_ = sz0_ % compsz0_ ? sz0_/compsz0_+1 : sz0_/compsz0_;
    nrcomp1_ = sz1_ % compsz1_ ? sz1_/compsz1_+1 : sz1_/compsz1_;
    nrcomp2_ = sz2_ % compsz2_ ? sz2_/compsz2_+1 : sz2_/compsz2_;
    
    const int nrcompartments = nrcomp0_*nrcomp1_*nrcomp2_;
    mAllocVarLenArr( int, arr, nrcompartments);
    for ( int idx=0; idx<nrcompartments; idx++ )
    {
	compartments_ += new Compartment();
	arr[idx] = idx;
    }
    
    std::random_shuffle( mVarLenArr(arr), arr+nrcompartments );
    for ( int idx=0; idx<nrcompartments; idx++ )
	permutation_ += arr[idx];
}


template <class T> inline
Array3DFloodfill<T>::~Array3DFloodfill()
{
    delete isdefined_;
    deepErase( compartments_ );
}


template <class T> inline
int Array3DFloodfill<T>::getWorkCompartment( int x0, int x1, int x2 ) const
{
    return (x0/compsz0_)*(x1/compsz1_)*(x2/compsz2_);
}


template <class T> inline
void Array3DFloodfill<T>::setOutsideValue( T val )
{
    if ( mIsUdf(val) )
	outsideval_ = val;
    else
    	outsideval_ = aboveisovalue_ ? val : -val;

    output_.setAll( outsideval_ );
}


template <class T> inline
void Array3DFloodfill<T>::addSeed( int x0, int x1, int x2 )
{
    setOutput( x0, x1, x2, true );
}


template <class T> inline
void Array3DFloodfill<T>::setOutput( int x0, int x1, int x2, bool addseed )
{
    if ( x0<0 || x0>=sz0_ || x1<0 || x1>=sz1_ || x2<0 || x2>=sz2_ )
	return;

    addToNrDone( 1 );

    const int cellidx = getWorkCompartment( x0, x1, x2 );

    compartments_[cellidx]->lock_.readLock();
    if ( isdefined_->get(x0,x1,x2) )
    {
	compartments_[cellidx]->lock_.readUnLock();
	return;
    }

    if ( !compartments_[cellidx]->lock_.convReadToWriteLock() )
    {
	if ( isdefined_->get(x0,x1,x2) )
	{
	    compartments_[cellidx]->lock_.writeUnLock();
	    return;
	}
    }

    if ( addseed )
    	isdefined_->set( x0, x1, x2, true );

    const T inputval = input_.get( x0, x1, x2 );

    if ( (aboveisovalue_ && inputval<=threshold_) || 
	 (!aboveisovalue_ && inputval>=threshold_) )
    {
	output_.set( x0, x1, x2, useinputval_ ? inputval : outsideval_ );
	compartments_[cellidx]->lock_.writeUnLock();
	return;
    }

    output_.set( x0, x1, x2, useinputval_ ? inputval : insideval_ );

    if ( addseed )
    {
	int dummy;
	int seed[] = { x0, x1, x2 };
	compartments_[cellidx]->seeds_.add( &dummy, seed );
	compartmentlock_.signal( false );
    }

    compartments_[cellidx]->lock_.writeUnLock();
}


template <class T> inline
int Array3DFloodfill<T>::getNextWorkCompartment()
{
    compartmentlock_.lock();
    while ( true )
    {
	bool nothingleft = true;
	for ( int idx=permutation_.size()-1; idx>=0; idx-- )
	{
	    const int compidx = permutation_[idx];
	    if ( compartments_[compidx]->isused_ )
		nothingleft = false;
	    else
	    {
		compartments_[compidx]->lock_.readLock();
		if ( !compartments_[compidx]->seeds_.isEmpty() )
		{
		    compartments_[compidx]->lock_.readUnLock();
		    compartments_[compidx]->isused_ = true;
		    compartmentlock_.unLock();
		    return compidx;
		}

		compartments_[compidx]->lock_.readUnLock();
	    }
	}

	if ( nothingleft )
	    break;

	compartmentlock_.wait();
    }

    compartmentlock_.unLock();
    return -1;
}


template <class T> inline
void Array3DFloodfill<T>::returnCompartment( int comp )
{
    compartmentlock_.lock();
    compartments_[comp]->isused_ = false;
    compartmentlock_.unLock();
    compartmentlock_.signal( true );
}


template <class T> inline
bool Array3DFloodfill<T>::doWork( od_int64 start, od_int64 stop, int )
{
    while ( shouldContinue() )
    {
	const int compidx = getNextWorkCompartment();
	if ( compidx==-1 )
	    return true;

	Compartment& comp = *compartments_[compidx];
	comp.lock_.readLock();
	while ( !comp.seeds_.isEmpty() )
	{
	    int idxs[3];
	    int arrpos[3];
	    comp.lock_.convReadToWriteLock();

	    if ( !comp.seeds_.getIndex( 0, idxs ) || 
		 !comp.seeds_.getPos( idxs, arrpos ) )
	    {
		comp.lock_.writeUnLock();
		returnCompartment( compidx );
		pErrMsg("Error!");
		return false;
	    }

	    comp.seeds_.remove( idxs );
	    comp.lock_.writeUnLock();

	    for ( int i=-1; i<=1; i++ )
	    {
		for ( int j=-1; j<=1; j++ )
		{
		    for ( int k=-1; k<=1; k++ )
		    {
			const char nrzeros = !i + !j + !k;
			if ( nrzeros==3 )
			    continue;

			const bool addseed = !use6neighbors_ || nrzeros==2;
			setOutput( arrpos[0]+i, arrpos[1]+j, arrpos[2]+k,
			           addseed );
		    }
		}
	    }

	    comp.lock_.readLock();
	}

	comp.lock_.readUnLock();
	returnCompartment( compidx );
    }

    return true;
}


#endif
