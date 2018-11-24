#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2010
________________________________________________________________________


-*/

#include "algomod.h"
#include "executor.h"
#include "arraynd.h"

/*!
\brief Gently smooths ArrayND by averaging with neighbours.

  The weight of the centre is always equal to the sum of the
  weights of the rest.

  The idea is to do this multiple times when stronger smoothing is required.
*/

template <class T>
mClass(Algo) ArrayNDGentleSmoother : public Executor
{ mODTextTranslationClass(ArrayNDGentleSmoother)
public:
			mTypeDefArrNDTypes;

			ArrayNDGentleSmoother(const ArrayND<T>&,
					      ArrayND<T>&);
    virtual		~ArrayNDGentleSmoother()	{}

    uiString		message() const	{ return uiStrings::sSmoothing(); }
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		nrDoneText() const
			{ return uiStrings::sPointsDone(); }

    int			nextStep();

protected:

    const ArrayND<T>&		inp_;
    ArrayND<T>&			out_;
    od_int64			totnr_;
    od_int64			nrdone_;
    const nr_dims_type		nrdims_;
    ArrayNDIter			it_;
    TypeSet<idx_type>		maxidxs_;

};


template <class T>
ArrayNDGentleSmoother<T>::ArrayNDGentleSmoother( const ArrayND<T>& inp,
						 ArrayND<T>& out )
    : Executor("Data smoother")
    , inp_(inp)
    , out_(out)
    , nrdims_(inp.nrDims())
    , it_(inp.info())
    , nrdone_(0)
{
    totnr_ = 1;
    for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	auto dimsz = inp_.getSize( idim );
	maxidxs_ += dimsz - 1;
	totnr_ *= dimsz;
    }
}


template <class T>
int ArrayNDGentleSmoother<T>::nextStep()
{
    NDPos itpos = it_.getPos();
    TypeSet<T> vals;

    for ( int idim=0; idim<nrdims_; idim++ )
    {
	for ( int idx=0; idx<2; idx++ )
	{
	    NDPosBuf pos( itpos, nrdims_ );
	    int& arridx = pos[idim];
	    if ( idx )
	    {
		arridx++;
		if ( arridx > maxidxs_[idim] )
		    arridx = maxidxs_[idim];
	    }
	    else
	    {
		arridx--;
		if ( arridx < 0 ) arridx = 0;
	    }
	    vals += inp_.getND( pos );
	}
    }

    T smval = 0; const auto nrvals = vals.size();
    const T val0 = inp_.getND( itpos );
    for ( int idx=0; idx<nrvals; idx++ )
	{ smval += val0; smval += vals[idx]; }
    smval /= 2 * nrvals;
    out_.setND( itpos, smval );
    nrdone_++;

    return it_.next() ? MoreToDo() : Finished();
}
