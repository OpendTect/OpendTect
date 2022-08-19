#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "paralleltask.h"
#include "position.h"
#include "trckeysampling.h"

template <class T> class Array2D;

/*!
\brief Base class for two dimensional array converter
*/

mExpClass(Algo) Array2DConverter : public ParallelTask
{ mODTextTranslationClass(Array2DConverter);
public:
    virtual			~Array2DConverter();

    Array2D<float>*		getOutput() const   { return arrout_; }

protected:
				Array2DConverter(const Array2D<float>&);

    const Array2D<float>&	arrin_;
    Array2D<float>*		arrout_		    = nullptr;
};


/*!
\brief Class for two dimensional array converter
from an XY grid to an inline/crossline grid
*/

mExpClass(Algo) Array2DFromXYConverter : public Array2DConverter
{ mODTextTranslationClass(Array2DFromXYConverter)
public:
				Array2DFromXYConverter(const Array2D<float>&,
						       const Coord& origin,
						       const Coord& step);
				~Array2DFromXYConverter();

    void			setOutputSampling(const TrcKeySampling&);

protected:
    int				maxNrThreads() const override;
    bool			doPrepare(int nrthreads) override;
    bool			doWork(od_int64,od_int64,int) override;
    od_int64			nrIterations() const override;
    uiString			uiNrDoneText() const override;

    TrcKeySampling		tks_;
    Coord			origin_;
    Coord			step_;
};
