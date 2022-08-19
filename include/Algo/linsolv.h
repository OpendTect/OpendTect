#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "executor.h"
#include "uistrings.h"
#include <math.h>

/*!
\brief LinSolver - Solves linear systems of equations on the form A*x=B. A is
a matrix of the size N*N, x is a column vector of the size N and B is a column
vector of the size N.
*/

template <class T>
mClass(Algo) LinSolver
{
public:
				LinSolver(const Array2D<T>& A);
				LinSolver(const LinSolver&);

				~LinSolver();

    bool			init(TaskRunner* =0);
    bool			ready() const	{ return ready_; }
    int				size() const	{ return n_; }

    void			apply(const T* b,T* x) const;

protected:
    Array2DImpl<T>		croutsmatrix_;
    int*			croutsidx_;
    int				n_;
    bool			parity_;
    bool			ready_;

};


template <class T>
mClass(Algo) LinSolverTask : public Executor
{ mODTextTranslationClass(LinSolverTask);
public:
				LinSolverTask(Array2DImpl<T>&,int*);
				~LinSolverTask()		{}

protected:

    int				nextStep() override;
    uiString			uiMessage() const override
				{
				    return errmsg_.isEmpty()
					? tr("Generating linear model")
					: errmsg_;
				}
    uiString			uiNrDoneText() const override
				{ return tr("Nr points processed"); }

    od_int64			nrDone() const override	{ return curidx_; }
    od_int64			totalNr() const override { return totalnr_; }

    int				curidx_;
    int				totalnr_;
    int				imax_;
    int*			croutsidx_;
    TypeSet<T>			vv_;
    Array2DImpl<T>&		croutsmatrix_;
    uiString			errmsg_;
};


template <class T>
LinSolverTask<T>::LinSolverTask( Array2DImpl<T>& A, int* croutsidx )
    : Executor("Generating linear model")
    , croutsmatrix_(A)
    , totalnr_(A.info().getSize(0))
    , curidx_(0)
    , imax_(mUdf(int))
    , vv_(totalnr_,0)
    , croutsidx_(croutsidx)
{
    if ( A.info().getSize(0) != A.info().getSize(1) )
    {
	errmsg_ = uiStrings::phrInvalid(uiStrings::sInput());
	return;
    }

    for ( int idx=0; idx<totalnr_; idx++ )
    {
	T maxval = 0;
	for ( int idy=0; idy<totalnr_; idy++ )
	{
	    const T temp = fabs( croutsmatrix_.get(idx,idy) );
	    if ( temp > maxval )
		maxval = temp;
	}

	if ( mIsZero(maxval,mDefEps) )
	{
	    errmsg_ = uiStrings::phrInvalid(uiStrings::sInput());
	    return;
	}

	vv_[idx] = 1.0f/maxval;
    }
}


#define TINY 1.0e-20

template <class T>
int LinSolverTask<T>::nextStep()
{
    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

    if ( curidx_ >= totalnr_ )
	return Finished();

    for (int idx=0; idx<curidx_; idx++ )
    {
	T sum = croutsmatrix_.get( idx, curidx_ );
	for ( int idz=0; idz<idx; idz++ )
	    sum -= croutsmatrix_.get(idx,idz) * croutsmatrix_.get(idz,curidx_);

	croutsmatrix_.set( idx, curidx_, sum );
    }

    T big = 0.0;
    for ( int idx=curidx_; idx<totalnr_; idx++ )
    {
	T sum = croutsmatrix_.get( idx, curidx_ );
	for ( int idz=0; idz<curidx_; idz++ )
	{
	    sum -= croutsmatrix_.get(idx,idz)*croutsmatrix_.get(idz,curidx_);
	}

	croutsmatrix_.set( idx, curidx_, sum );

	T dum = vv_[idx]*fabs(sum);

	if ( dum >= big )
	{
	    big = dum;
	    imax_ = idx;
	}
    }

    if ( curidx_ != imax_ )
    {
	for ( int idz=0; idz<totalnr_; idz++ )
	{
	    const T dum = croutsmatrix_.get( imax_, idz );
	    croutsmatrix_.set( imax_, idz, croutsmatrix_.get(curidx_,idz) );
	    croutsmatrix_.set( curidx_, idz, dum );
	}

	vv_[imax_] = vv_[curidx_];
    }

    croutsidx_[curidx_] = imax_;

    if ( mIsZero(croutsmatrix_.get(curidx_,curidx_),mDefEps) )
	croutsmatrix_.set(curidx_,curidx_,TINY);

    if ( curidx_ != totalnr_-1 )
    {
	const T dum = 1.0f/croutsmatrix_.get(curidx_,curidx_);
	for ( int idx=curidx_+1; idx<totalnr_; idx++ )
	    croutsmatrix_.set(idx, curidx_, dum*croutsmatrix_.get(idx,curidx_));
    }

    return ++curidx_>=totalnr_ ? Finished() : MoreToDo();
}

#undef TINY


template <class T> inline
LinSolver<T>::LinSolver( const Array2D<T>& A )
    : croutsmatrix_(A)
    , croutsidx_(new int[A.info().getSize(0)])
    , n_(A.info().getSize(0))
    , ready_(false)
    , parity_(true)
{
}


template <class T> inline
bool LinSolver<T>::init( TaskRunner* taskr )
{
    LinSolverTask<T> task( croutsmatrix_, croutsidx_ );
    ready_ = TaskRunner::execute( taskr, task );
    return ready_;
}


template <class T> inline
LinSolver<T>::LinSolver( const LinSolver& s )
    : croutsmatrix_(s.croutsmatrix_)
    , croutsidx_(0)
    , n_(s.n_)
    , parity_(s.parity_)
    , ready_(s.ready_)
{
    if ( s.croutsidx_ )
    {
	croutsidx_ = new int[s.n_];
	for ( int idx=0; idx<s.n_; idx++ )
	    croutsidx_[idx] = s.croutsidx_[idx];
    }
}


template <class T> inline
LinSolver<T>::~LinSolver( )
{
    delete [] croutsidx_;
}


template <class T> inline
void LinSolver<T>::apply( const T* b, T* x ) const
{
    if ( !ready_ )
    {
	pErrMsg("Cannot apply. Did you forget to call LinSolver::init()?");
	return;
    }

    for ( int idx=0; idx<n_; idx++ )
	x[idx] = b[idx];

    int ii=-1;

    for ( int i=0; i<n_; i++ )
    {
	int ip=croutsidx_[i];
	T sum=x[ip];
	x[ip]=x[i];

	if ( ii != -1 )
	{
	    for ( int j=ii; j<=i-1; j++ )
	    {
		sum -= croutsmatrix_.get(i,j)*x[j];
	    }
	}
	else if ( !mIsZero(sum,mDefEps) )
	    ii=i;

	x[i]=sum;
    }

    for ( int i=n_-1; i>=0; i-- )
    {
	T sum=x[i];
	for ( int j=i+1; j<n_; j++ )
	    sum -= croutsmatrix_.get(i,j)*x[j];

	x[i]=sum/croutsmatrix_.get(i,i);
    }

}
