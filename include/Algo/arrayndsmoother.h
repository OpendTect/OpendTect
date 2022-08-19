#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

			ArrayNDGentleSmoother(const ArrayND<T>&,
					      ArrayND<T>&);
    virtual		~ArrayNDGentleSmoother()	{}

    uiString		uiMessage() const override
			{ return tr("Smoothing"); }
    od_int64		totalNr() const override	{ return totnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Points handled"); }

    int			nextStep() override;

protected:

    const ArrayND<T>&		inp_;
    ArrayND<T>&			out_;
    od_int64			totnr_;
    od_int64			nrdone_;
    const int			nrdims_;
    ArrayNDIter			it_;
    TypeSet<int>		maxidxs_;

};


template <class T>
ArrayNDGentleSmoother<T>::ArrayNDGentleSmoother( const ArrayND<T>& inp,
						 ArrayND<T>& out )
    : Executor("Data smoother")
    , inp_(inp)
    , out_(out)
    , nrdims_(inp.info().getNDim())
    , it_(inp.info())
    , nrdone_(0)
{
    totnr_ = 1;
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	int dimsz = inp_.info().getSize(idim);
	maxidxs_ += dimsz - 1;
	totnr_ *= dimsz;
    }
}


template <class T>
int ArrayNDGentleSmoother<T>::nextStep()
{
    const int* itpos = it_.getPos();
    TypeSet<T> vals;

    for ( int idim=0; idim<nrdims_; idim++ )
    {
	for ( int idx=0; idx<2; idx++ )
	{
	    TypeSet<int> arridxs( itpos, nrdims_ );
	    int& arridx = arridxs[idim];
	    if ( idx )
	    {
		arridx++;
		if ( arridx > maxidxs_[idim] ) arridx = maxidxs_[idim];
	    }
	    else
	    {
		arridx--;
		if ( arridx < 0 ) arridx = 0;
	    }
	    vals += inp_.getND( arridxs.arr() );
	}
    }

    T smval = 0; const int nrvals = vals.size();
    const T val0 = inp_.getND( itpos );
    for ( int idx=0; idx<nrvals; idx++ )
	{ smval += val0; smval += vals[idx]; }
    smval /= 2 * nrvals;
    out_.setND( itpos, smval );
    nrdone_++;

    return it_.next() ? MoreToDo() : Finished();
}
