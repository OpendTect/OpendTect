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
    void			setExtrapol( bool yn )
				{ doextrapol_ = yn; }
    void			setFillWithExtremes( bool yn )
				{ fillwithextremes_ = yn; }
    void			reset();

protected:
				Array1DInterpol();
    const char*			nrDoneText() const
    				{ return "Points interpolated "; }
    Array1D<float>*	arr_;
    bool		arrstarted_;
    bool		doextrapol_;
    bool		fillwithextremes_; //extrapolate with last valid values
    int			maxgapsize_;
    unsigned int	nrdone_;

};


/*!
\brief Does linear interpolation of one dimensional arrays.
*/

mExpClass(Algo) LinearArray1DInterpol : public Array1DInterpol
{
public:
    				LinearArray1DInterpol();
protected:
    int				nextStep();
    void			extrapolate(bool start);
};


/*!
\brief Uses a 3rd degree polynomial for interpolation of one dimensional arrays.
*/

mExpClass(Algo) PolyArray1DInterpol : public Array1DInterpol
{
public:
    				PolyArray1DInterpol();
protected:
    int				nextStep();
    bool			getPositions(int pos,TypeSet<float>& posidxs);
    void			extrapolate(bool start);
};


#endif

