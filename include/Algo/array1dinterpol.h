#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "executor.h"

template <class T> class Array1D;


/*!
\brief Base class for one dimensional array interpolators.
*/

mExpClass(Algo) Array1DInterpol : public Executor
{ mODTextTranslationClass(Array1DInterpol);
public:
    virtual		~Array1DInterpol();

    void		setMaxGapSize(float);
    float		getMaxGapSize() const;

    void		setArray(Array1D<float>&);

    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		nrIterations() const;
    void		setExtrapol( bool yn )	{ doextrapol_ = yn; }
    void		setFillWithExtremes( bool yn )
			{ fillwithextremes_ = yn; }
    void		reset();

protected:
			Array1DInterpol();
    uiString		uiNrDoneText() const override
			{ return tr("Points interpolated"); }

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
    int			nextStep() override;
    bool		doFinish(bool,od_ostream* =nullptr) override;
    void		extrapolate(bool start);
};


/*!
\brief Uses a 3rd degree polynomial for interpolation of one dimensional arrays.
*/

mExpClass(Algo) PolyArray1DInterpol : public Array1DInterpol
{
public:
			PolyArray1DInterpol();
protected:
    int			nextStep() override;
    bool		doFinish(bool,od_ostream* =nullptr) override;
    bool		getPositions(int pos,TypeSet<float>& posidxs);
    void		extrapolate(bool start);
};
