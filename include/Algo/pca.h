#ifndef pca_h
#define pca_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: pca.h,v 1.5 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "trigonometry.h"

template <class T> class Array2D;
namespace Threads { class ThreadWorkManager; };
class BasicTask;
class PCACovarianceCalculator;

/*!\brief
Performs Pricipal Component Analysis on samples with N variables.
*/

class PCA
{
public:
    					PCA( int nrvars );
    virtual				~PCA();

    template <class IDXABL> void	addSample( const IDXABL& );
    void				clearAllSamples();

    bool				calculate();

    float			getEigenValue(int) const;
    void			getEigenVector(int, TypeSet<float>&) const;
    void			setThreadWorker ( Threads::ThreadWorkManager* );

protected:
    bool			tqli( float[], float[], int, ObjectSet<float>&);
    void			tred2( ObjectSet<float>&, int, float[],float[]);

    const int			nrvars;
    Array2D<float>&		covariancematrix;
    ObjectSet<TypeSet<float> >	samples;
    TypeSet<float>		samplesums;
    Threads::ThreadWorkManager*	threadworker;
    ObjectSet<BasicTask>	tasks;
    TypeSet<float>		eigenvalues;
};


template <class IDXABL>
void PCA::addSample( const IDXABL& sample )
{
    TypeSet<float>& ownsample = *new TypeSet<float>;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	const float val = sample[idx];
	ownsample += val;
	samplesums[idx] += val;
    }

    samples += &ownsample;
}

#endif

