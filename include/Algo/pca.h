#ifndef pca_h
#define pca_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: pca.h,v 1.6 2003-11-24 08:36:13 kristofer Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
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
    template <class IDXABL> void getEigenVector(int, IDXABL&) const;
    void			setThreadWorker ( Threads::ThreadWorkManager* );

protected:
    bool			tqli( float[], float[], int, ObjectSet<float>&);
    void			tred2( ObjectSet<float>&, int, float[],float[]);

    const int			nrvars;
    Array2DImpl<float>		covariancematrix;
    ObjectSet<TypeSet<float> >	samples;
    TypeSet<float>		samplesums;
    Threads::ThreadWorkManager*	threadworker;
    ObjectSet<BasicTask>	tasks;
    float*			eigenvalues;
    				/*!<The negation of the eigenval,
    				    to get the sorting right.
				*/
    int*			eigenvecindexes;
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

template <class IDXABL>
void PCA::getEigenVector(int idy, IDXABL& vec ) const
{
    for ( int idx=0; idx<nrvars; idx++ )
	vec[idx] = covariancematrix.get(idx, eigenvecindexes[idy] );
}

#endif

