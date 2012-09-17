#ifndef array2dfilter_h
#define array2dfilter_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Nov 2006
 RCS:           $Id: array2dfilter.h,v 1.14 2011/10/26 14:20:13 cvsbruno Exp $
________________________________________________________________________


@$*/

#include "arrayndimpl.h"
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
    inline		Array2DFilterer(const Array2D<T>&,
	    				Array2D<T>&, const RowCol& origin,
	    				const Array2DFilterPars&);
    inline		~Array2DFilterer();

    inline void		setScope(const Interval<int>& rowrg,
	    			 const Interval<int>& colrg);

    inline Array2D<T>&	output()		{ return output_; }
    inline const Array2D<T>& output() const	{ return output_; }

    inline const Array2DFilterPars& pars() const { return pars_; }
    inline const Stats::RunCalc<float>& calc() const { return *calc_; }

    inline int		nextStep();
    inline const char*	message() const		{ return "Filtering data"; }
    inline od_int64	nrDone() const		{ return nrrowsdone_; }
    inline const char*	nrDoneText() const	{ return "Columns handled";}
    inline od_int64	totalNr() const		{return outputrowrg_.width()+1;}

protected:

    Array2D<T>&		output_;
    Array2DFilterPars	pars_;
    Stats::RunCalc<float>* calc_;

    Array2DImpl<T>	input_;
    RowCol		origin_;

    Interval<int>	outputrowrg_;
    Interval<int>	outputcolrg_;

    const int		inputrowsize_;
    const int		inputcolsize_;
    const int		nrcols_;
    bool		linefilt_;
    int			nrrowsdone_;

    inline void		filterRow(int);
    inline void		doPoint(int,int);

};


template <class T> inline
Array2DFilterer<T>::Array2DFilterer( Array2D<T>& a, const Array2DFilterPars& p )
    : Executor("2D Filtering")
    , input_( a )
    , pars_(p)
    , output_(a)
    , calc_(0)
    , inputrowsize_(a.info().getSize(0))
    , inputcolsize_(a.info().getSize(1))
    , nrcols_(2 * p.stepout_.col + 1)
    , nrrowsdone_(0)
    , origin_( 0, 0 )
{
    outputrowrg_.start = 0; outputrowrg_.stop = inputrowsize_-1;
    outputcolrg_.start = 0; outputcolrg_.stop = inputcolsize_-1;

    Stats::CalcSetup setup( !mIsUdf(pars_.rowdist_) );
    setup.require( pars_.type_ );
    calc_ = new Stats::RunCalc<float>( setup );
}


template <class T> inline
Array2DFilterer<T>::Array2DFilterer( const Array2D<T>& input, Array2D<T>& a,
				    const RowCol& origin,
				    const Array2DFilterPars& p )
    : Executor("2D Filtering")
    , input_( input )
    , pars_(p)
    , output_(a)
    , calc_(0)
    , inputrowsize_(input.info().getSize(0))
    , inputcolsize_(input.info().getSize(1))
    , nrcols_(2 * p.stepout_.col + 1)
    , nrrowsdone_(0)
    , origin_( origin )
{
    outputrowrg_.start = origin.row;
    outputrowrg_.stop = origin.row+a.info().getSize(0)-1;

    outputcolrg_.start = origin.col;
    outputcolrg_.stop = origin.col+a.info().getSize(1)-1;

    Stats::CalcSetup setup( !mIsUdf(pars_.rowdist_) );
    setup.require( pars_.type_ );
    calc_ = new Stats::RunCalc<float>( setup );
}


template <class T> inline
Array2DFilterer<T>::~Array2DFilterer()
{ }


template <class T>
inline void Array2DFilterer<T>::setScope(const Interval<int>& rowrg,
					 const Interval<int>& colrg)
{
    outputrowrg_ = rowrg;
    outputcolrg_ = colrg;
}


template <class T> inline int Array2DFilterer<T>::nextStep()
{
    if ( !nrrowsdone_ )
    {
	if ( !input_.getData() )
	{
	    ErrMsg("Memory full");
	    return ErrorOccurred();
	}

	if ( inputrowsize_ < 2 * pars_.stepout_.row + 1
	  || inputcolsize_ < 2 * pars_.stepout_.col + 1
	  || (pars_.stepout_.col < 1 && pars_.stepout_.row < 1) )
	{
	    ErrMsg("Invalid parameters");
	    return ErrorOccurred();
	}

	linefilt_ = pars_.stepout_.row == 0 || pars_.stepout_.col == 0;
    }


    const int currow = outputrowrg_.start + nrrowsdone_;
    if ( currow>outputrowrg_.stop )
	return Executor::Finished();

    filterRow( currow );
    nrrowsdone_++;
    return Executor::MoreToDo();
}


template <class T> inline void Array2DFilterer<T>::filterRow( int row )
{
    const T* inputptr = pars_.filludf_
	? 0
	: input_.getData() + input_.info().getOffset( row, 0 );

    if ( inputptr )
    {
	for ( int col=outputcolrg_.start; col<=outputcolrg_.stop;
	      col++, inputptr++ )
	{
	    if ( !mIsUdf(*inputptr) )
		doPoint( row, col );
	}

	return;
    }

    for ( int col=outputcolrg_.start; col<=outputcolrg_.stop; col++ )
	doPoint( row, col );
}


template <class T>
inline void Array2DFilterer<T>::doPoint( int row, int col )
{
    calc_->clear();

    const bool isweighted = calc_->isWeighted();
    const int startrow = row - pars_.stepout_.row;
    int firstrow = startrow;
    if ( firstrow < 0 ) firstrow = 0;
    const int endrow = row + pars_.stepout_.row;
    int lastrow = endrow;
    if ( lastrow >= inputrowsize_ ) lastrow = inputrowsize_-1;

    const int startcol = col - pars_.stepout_.col;
    int firstcol = startcol;
    if ( firstcol < 0 ) firstcol = 0;
    const int endcol = col + pars_.stepout_.col;
    int lastcol = endcol;
    if ( lastcol >= inputcolsize_ ) lastcol = inputcolsize_-1;

    for ( int irow=firstrow; irow<=lastrow; irow++ )
    {
	const bool issiderow = irow == startrow || irow == endrow;
	const int rowdist = row-irow;
	const int rowdist2 = rowdist*rowdist;
	const bool iscenterrow = irow == row;

	const T* buf = input_.getData()+input_.info().getOffset(irow,firstcol);

	for ( int icol=firstcol; icol<=lastcol; icol++, buf++ )
	{
	    const bool issidecol = icol == startcol || icol == endcol;
	    const bool iscentercol = icol == col;

	    if ( !linefilt_ && ( (issidecol && !iscenterrow )
		|| (issiderow && !iscentercol) ) )
		continue;

	    if ( isweighted )
	    {
		const int coldist = icol - col;
		const int coldist2 = coldist*coldist;
		float wt = pars_.rowdist_ * rowdist2 + coldist2 ;
		wt = 1. / (1 + pars_.distfac_ * wt);
		calc_->addValue( *buf, wt );
	    }
	    else
		*calc_ += *buf;
	}
    }

    output_.set( row-origin_.row, col-origin_.col,calc_->getValue(pars_.type_));
}


#endif
