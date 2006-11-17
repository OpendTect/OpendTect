#ifndef array2dfilter_h
#define array2dfilter_h

/*@+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Nov 2006
 RCS:           $Id: array2dfilter.h,v 1.2 2006-11-17 13:09:41 cvsbert Exp $
________________________________________________________________________


@$*/

#include "arraynd.h"
#include "statruncalc.h"
#include "executor.h"
#include "iopar.h"
#include "rowcol.h"
#include "errh.h"
#include <math.h>

/*!\brief Parameters for Array2DFilterer

  The filtering can be done weighted, on 1 / (1 + distfac_*square of distance).
  If you want that, set the rowdist_ to non-undef. The value should then be
  the distance between two rows if the distance between cols is 1.

 */

struct Array2DFilterPars
{

			Array2DFilterPars( Stats::Type t=Stats::Average,
					   RowCol rc=RowCol(1,1),
			       		   bool filludf=false )
			: type_(t)
			, stepout_(rc)
			, filludf_(filludf)
			, rowdist_(mUdf(float))
			, distfac_(1)		{}

    Stats::Type		type_;
    RowCol		stepout_;	//!< In nodes. Center point not counted
    bool		filludf_;	//!< Output when center point is undef?
    float		rowdist_;	//!< non-undef = weighted filter
    float		distfac_;	//!< if weigthed, distance factor

};



/*!\brief Filters an Array2D

  Note that you cannot change the filter pars after construction.

 */

template <class T>
class Array2DFilterer : public Executor
{
public:

    inline		Array2DFilterer(Array2D<T>&,const Array2DFilterPars&);
    inline		~Array2DFilterer();

    inline Array2D<T>&	arr()			{ return arr_; }
    inline const Array2D<T>& arr() const	{ return arr_; }
    inline const Array2DFilterPars& pars() const { return pars_; }
    inline const Stats::RunCalc<float>& calc() const { return *calc_; }

    inline int		nextStep();
    inline const char*	message() const		{ return "Filtering data"; }
    inline int		nrDone() const		{ return nrcolsdone_; }
    inline const char*	nrDoneText() const	{ return "Columns handled";}
    inline int		totalNr() const		{ return colsize_; }

protected:

    Array2D<T>&		arr_;
    Array2DFilterPars	pars_;
    Stats::RunCalc<float>* calc_;
    T**			bufs_;
    int*		idxs_;

    const int		rowsize_;
    const int		colsize_;
    const int		nrcols_;
    const int		nrrows_;
    int			nrcolsdone_;

    inline void		releaseAll();
    inline bool		manageBufs(int);
    inline void		filterCol();
    inline void		doPoint(int);

};


template <class T> inline
Array2DFilterer<T>::Array2DFilterer( Array2D<T>& a, const Array2DFilterPars& p )
    : Executor("2D Filtering")
    , pars_(p)
    , arr_(a)
    , bufs_(0)
    , calc_(0)
    , rowsize_(a.info().getSize(0))
    , colsize_(a.info().getSize(1))
    , nrcols_(2 * p.stepout_.c() + 1)
    , nrrows_(2 * p.stepout_.r() + 1)
    , nrcolsdone_(0)
{
    if ( rowsize_ < 2 * p.stepout_.r() + 1
      || colsize_ < 2 * p.stepout_.c() + 1
      || (pars_.stepout_.c() < 1 && pars_.stepout_.r() < 1) )
	{ nrcolsdone_ = colsize_; return; }

    bufs_ = new T* [ nrcols_ ];
    idxs_ = new int [ nrcols_ ];
    for ( int idx=0; idx<nrcols_; idx++ )
    {
	bufs_[idx] = new T [rowsize_];
	idxs_[idx] = idx;
	int inpcol = idx < pars_.stepout_.c() ? 0 : idx - pars_.stepout_.c();
	if ( inpcol >= colsize_ ) inpcol = colsize_ - 1;
	for ( int irow=0; irow<rowsize_; irow++ )
	    bufs_[idx][irow] = arr_.get( irow, inpcol );
    }
    if ( !bufs_[nrcols_-1] )
	{ releaseAll(); ErrMsg("Memory full"); nrcolsdone_ = colsize_; return; }

    Stats::RunCalcSetup setup( !mIsUdf(pars_.rowdist_) );
    setup.require( pars_.type_ );
    calc_ = new Stats::RunCalc<float>( setup );
}


template <class T> inline
Array2DFilterer<T>::~Array2DFilterer()
{
    releaseAll();
}


template <class T> inline
void Array2DFilterer<T>::releaseAll()
{
    delete calc_; calc_ = 0;
    if ( !bufs_ ) return;

    for ( int idx=0; idx<nrcols_; idx++ )
	delete [] bufs_[idx];
    delete [] bufs_; bufs_ = 0;
}


template <class T> inline int Array2DFilterer<T>::nextStep()
{
    if ( !bufs_ || nrcolsdone_ >= colsize_ )
	return Executor::Finished;

    if ( !manageBufs(nrcolsdone_) )
	return Executor::Finished;

    filterCol();
    nrcolsdone_++;
    return Executor::MoreToDo;
}


template <class T> inline bool Array2DFilterer<T>::manageBufs( int col )
{
    if ( col == 0 || col >= colsize_ )
	return false;

    T* fillbuf;
    for ( int idx=0; idx<nrcols_; idx++ )
    {
	if ( idxs_[idx] != 0 )
	    idxs_[idx]--;
	else
	{
	    idxs_[idx] = nrcols_ - 1;
	    fillbuf = bufs_[idx];
	}
    }

    int inpcol = col + pars_.stepout_.c();
    if ( inpcol >= colsize_ )
	inpcol = colsize_ - 1;

    for ( int irow=0; irow<rowsize_; irow++ )
	fillbuf[irow] = arr_.get( irow, inpcol );

    return true;
}


template <class T> inline void Array2DFilterer<T>::filterCol()
{
    for ( int irow=0; irow<rowsize_; irow++ )
    {
	if ( !pars_.filludf_ )
	{
	    const T v00 = bufs_[ idxs_[nrcols_/2] ][irow];
	    if ( mIsUdf(v00) )
		continue;
	}
	doPoint( irow );
    }
}


template <class T>
inline void Array2DFilterer<T>::doPoint( int row )
{
    calc_->clear();

    int startrow = row - pars_.stepout_.r();
    if ( startrow < 0 ) startrow = 0;
    int endrow = row + pars_.stepout_.r();
    if ( endrow <= nrrows_ ) endrow = nrrows_-1;

    const int centercol = idxs_[colsize_/2];
    for ( int icol=0; icol<nrcols_; icol++ )
    {
	const int realcol = idxs_[icol];
	const T* buf = bufs_[ idxs_[icol] ];
	for ( int irow=startrow; irow<=endrow; irow++ )
	{
	    if ( !calc_->isWeighted() )
		*calc_ += buf[irow];
	    else
	    {
		float wt = (row - irow) * (row - irow)
		    	 + (centercol - realcol) * (centercol - realcol);
		wt = 1 / (1 + pars_.distfac_ * wt);
		calc_->addValue( buf[irow], wt );
	    }
	}
	arr_.set( row, realcol, calc_->getValue(pars_.type_) );
    }
}


#endif
