#ifndef pca_h
#define pca_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "sets.h"
#include "trigonometry.h"
#include "threadwork.h"

template <class T> class Array2D;
class SequentialTask;
class PCACovarianceCalculator;

/*!
  \ingroup Algo
  \brief
  Performs Pricipal Component Analysis on samples with N variables.
  
  Example of usage:
  \code
  //The samples will have three variables
  PCA pca( 3 );		
  
  //Samples can be added by any object that has a readable [] operator
  const float sample0[] = { 0, 1, 2 };
  pca.addSample( sample0 );
  
  const float sample1[] = { 4.343, 9.8, 2.72 };
  pca.addSample( sample1 );
  
  const float sample2[] = { 23.15, 210, -15 };
  pca.addSample( sample2 );
  
  const float sample3[] = { -0.36, 0.68, 3 };
  pca.addSample( sample3 );
  
  const float sample4[] = { 4.4, 9,6, 11 };
  pca.addSample( sample4 );
  
  TypeSet<float> sample5; sample5 += 34.1; sample5 += 8.37; sample5 += -44;
  pca.addSample( sample5 ); 
  
  pca.calculate();
  
  
  //Any object that has a writable [] operator can be used to fetch
  //the resulting vectors:
  TypeSet<float> eigenvec0(3,0);
  float eigenval0 = pca.getEigenValue(0);
  pca.getEigenVector( 0, eigenvec0 );
  
  float[3] eigenvec1;
  float eigenval1 = pca.getEigenValue(1);
  pca.getEigenVector( 1, eigenvec1 );
  
  float[3] eigenvec2;
  float eigenval2 = pca.getEigenValue(2);
  pca.getEigenVector( 2, eigenvec2 );
  \endcode
*/

mClass(Algo) PCA
{
public:
    					PCA( int nrvars );
					/*!<\param nrvars The number of
						  variables that the samples
						  have. */
    virtual				~PCA();

    void				clearAllSamples();
    					/*!< Removes all samples so a new
					     analysis can be made (by adding
					     new samples) */
    template <class IDXABL> void	addSample( const IDXABL& sample );
    					/*!<Adds a sample to the analysis.
					    \param sample The sample that should
					    		  be added. The sample
							  can be of any type
							  that have []
							  operators.  */
    bool				calculate();
    					/*!< Computes the pca for all
					     added samples. */
    float				getEigenValue(int idx) const;
    					/*!<\return an eigenvalue.
					  \param idx Determines which 
					  	eigenvalue to return. The
						eigenvalues are sorted in
						descending order, so idx==0
						gives the largest eigenvalue. */
    template <class IDXABL> void	getEigenVector(int idx,
	    					       IDXABL& vec) const;
    					/*!<Returns the eigenvector
					    corresponding to the eigenvalue
					    \a idx.
					    \param idx Determines which
					    	eigenvector to return. The
						eigenvectors are sorted in
						order of descending eigenvalue,
						so idx==0 gives the eigenvector
						corresponding to the largest
						eigenvalue.
					    \param vec The object where the to
					     	       store the eigenvector.
						       Any object that has a
						       writable [] operator can
						       be used, for example
						       float* and TypeSet<T>.
					*/
protected:
    bool			tqli( float[], float[], int, ObjectSet<float>&);
    void			tred2( ObjectSet<float>&, int, float[],float[]);

    const int			nrvars_;
    Array2DImpl<float>		covariancematrix_;
    ObjectSet<TypeSet<float> >	samples_;
    TypeSet<float>		samplesums_;
    TypeSet<Threads::Work>	workload_;
    ObjectSet<SequentialTask>	tasks_;
    float*			eigenvalues_;
    				/*!<The negation of the eigenval,
    				    to get the sorting right.
				*/
    int*			eigenvecindexes_;
};


template <class IDXABL> inline
void PCA::addSample( const IDXABL& sample )
{
    TypeSet<float>& ownsample = *new TypeSet<float>;
    for ( int idx=0; idx<nrvars_; idx++ )
    {
	const float val = (float) sample[idx];
	ownsample += val;
	samplesums_[idx] += val;
    }

    samples_ += &ownsample;
}

template <class IDXABL> inline
void PCA::getEigenVector(int idy, IDXABL& vec ) const
{
    for ( int idx=0; idx<nrvars_; idx++ )
	vec[idx] = covariancematrix_.get(idx, eigenvecindexes_[idy] );
}

#endif


