#ifndef pca_h
#define pca_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: pca.h,v 1.1 2002-12-30 08:57:32 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "trigonometry.h"

template <class T> class Array2D;
namespace Threads { class ThreadWorkManager; };
class BasicTask;

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

    void				calculate();

    float				getEigenValue(int);
    Vector3				getEigenVector(int) const;

protected:
    const int			nrvars;
    Array2D<float>&		covariancematrix;
    ObjectSet<TypeSet<float> >	samples;
    TypeSet<float>		samplesums;
    Threads::ThreadWorkManager*	threadworker;
    ObjectSet<BasicTask>	tasks;
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

