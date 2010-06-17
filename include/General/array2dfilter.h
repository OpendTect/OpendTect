#ifndef array2dfilter_h
#define array2dfilter_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Nov 2006
 RCS:           $Id: array2dfilter.h,v 1.9 2010-06-17 21:59:48 cvskris Exp $
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
    inline od_int64	nrDone() const		{ return nrcolsdone_; }
    inline const char*	nrDoneText() const	{ return "Columns handled";}
    inline od_int64	totalNr() const		{ return colsize_; }

protected:

    Array2D<T>&		arr_;
    Array2DFilterPars	pars_;
    Stats::RunCalc<float>* calc_;
    T**			bufs_;
    int*		colnrs_;

    const int		rowsize_;
    const int		colsize_;
    const int		nrcols_;
    const bool		linefilt_;
    int			nrcolsdone_;

    inline void		releaseAll();
    inline bool		manageBufs(int);
    inline void		filterCol(int);
    inline void		doPoint(int,int,int);

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
    , nrcols_(2 * p.stepout_.col + 1)
    , linefilt_(p.stepout_.row == 0 || p.stepout_.col == 0)
    , nrcolsdone_(0)
{
    if ( rowsize_ < 2 * p.stepout_.row + 1
      || colsize_ < 2 * p.stepout_.col + 1
      || (pars_.stepout_.col < 1 && pars_.stepout_.row < 1) )
	{ nrcolsdone_ = colsize_; return; }

    bufs_ = new T* [ nrcols_ ];
    colnrs_ = new int [ nrcols_ ];
    int iidx = 0;
    for ( int idx=0; idx<nrcols_; idx++ )
    {
	bufs_[idx] = new T [rowsize_];
	colnrs_[idx] = idx - pars_.stepout_.col;
	if ( colnrs_[idx] >= 0 && colnrs_[idx] < colsize_ )
	{
	    for ( int irow=0; irow<rowsize_; irow++ )
		bufs_[idx][irow] = arr_.get( irow, colnrs_[idx] );
	}
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
	return Executor::Finished();

    if ( !manageBufs(nrcolsdone_) )
	return Executor::Finished();

    filterCol( nrcolsdone_ );
    nrcolsdone_++;
    return Executor::MoreToDo();
}


template <class T> inline bool Array2DFilterer<T>::manageBufs( int col )
{
    if ( col == 0 )
	return true;
    else if ( col >= colsize_ )
	return false;

    int mincol = colnrs_[0]; int maxcol = mincol;
    int mincolidx = 0;
    for ( int idx=1; idx<nrcols_; idx++ )
    {
	if ( colnrs_[idx] < mincol )
	    { mincol = colnrs_[idx]; mincolidx = idx; }
	if ( colnrs_[idx] > maxcol )
	    maxcol = colnrs_[idx];
    }

    const int newcolnr = maxcol + 1;
    colnrs_[mincolidx] = newcolnr;
    if ( newcolnr >= colsize_ )
	return true;

    T* fillbuf = bufs_[mincolidx];
    for ( int irow=0; irow<rowsize_; irow++ )
	fillbuf[irow] = arr_.get( irow, newcolnr );

    return true;
}


template <class T> inline void Array2DFilterer<T>::filterCol( int col )
{
    int colidx = 0;
    for ( int icol=0; icol<nrcols_; icol++ )
    {
	if ( colnrs_[icol] == col )
	    { colidx = icol; break; }
    }

    for ( int irow=0; irow<rowsize_; irow++ )
    {
	if ( pars_.filludf_ || !mIsUdf(bufs_[colidx][irow]) )
	    doPoint( irow, col, colidx );
    }
}


template <class T>
inline void Array2DFilterer<T>::doPoint( int row, int col, int colidx )
{
    calc_->clear();

    int startrow = row - pars_.stepout_.row;
    if ( startrow < 0 ) startrow = 0;
    int endrow = row + pars_.stepout_.row;
    if ( endrow >= rowsize_ ) endrow = rowsize_-1;

    for ( int icol=0; icol<nrcols_; icol++ )
    {
	if ( colnrs_[icol] < 0 || colnrs_[icol] >= colsize_ )
	    continue;

	const bool issidecol = colnrs_[icol] == col - pars_.stepout_.col
			    || colnrs_[icol] == col + pars_.stepout_.col;
	const bool iscentercol = colnrs_[icol] == col;

	const T* buf = bufs_[ icol ];
	const int coldist = colnrs_[icol] - colnrs_[colidx];

	for ( int irow=startrow; irow<=endrow; irow++ )
	{
	    const bool issiderow = irow == startrow || irow == endrow;
	    if ( !linefilt_
	      && ( (issidecol && irow != row)
		|| (issiderow && !iscentercol) ) )
		continue;

	    if ( !calc_->isWeighted() )
		*calc_ += buf[irow];
	    else
	    {
		float wt = pars_.rowdist_ * (row - irow) * (row - irow)
		         + coldist * coldist;
		wt = 1 / (1 + pars_.distfac_ * wt);
		calc_->addValue( buf[irow], wt );
	    }
	}
    }

    arr_.set( row, col, calc_->getValue(pars_.type_) );
}


#endif
