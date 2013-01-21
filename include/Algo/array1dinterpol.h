#ifndef array1dinterpol_h
#define array1dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          December 2009
 RCS:           $Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "executor.h"

template <class T> class Array1D;


/*!
\ingroup Algo
\brief Base class for one dimensional array interpolators.
*/

mExpClass(Algo) Array1DInterpol : public Executor
{
public:
    virtual			~Array1DInterpol();

    void			setMaxGapSize(float);
    float			getMaxGapSize() const;

    void			setArray(Array1D<float>&);

    od_int64			nrDone() const 		{ return nrdone_; }
    od_int64			nrIterations() const;

protected:
				Array1DInterpol();
    const char*			nrDoneText() const
    				{ return "Points interpolated "; }
    Array1D<float>*	arr_;
    bool		arrstarted_;
    int			maxgapsize_;
    unsigned int	nrdone_;

};


/*!
\ingroup Algo
\brief Does linear interpolation of one dimensional arrays.
*/

mExpClass(Algo) LinearArray1DInterpol : public Array1DInterpol
{
public:
    				LinearArray1DInterpol();
protected:
    int				nextStep();
};


/*!
\ingroup Algo
\brief Uses a 3rd degree polynomial for interpolation of one dimensional arrays.
*/

mExpClass(Algo) PolyArray1DInterpol : public Array1DInterpol
{
public:
    				PolyArray1DInterpol();
protected:
    int				nextStep();
    bool			getPositions(int pos,TypeSet<float>& posidxs);
};


#endif

