/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "pca.h"

#include "task.h"
#include "errh.h"
#include "sorting.h"
#include "thread.h"
#include "threadwork.h"

#include <math.h>


class PCACovarianceCalculator : public SequentialTask
{
public:
    		PCACovarianceCalculator( Array2D<float>& covariancematrix,
					 int row, int col,
					 ObjectSet<TypeSet<float> >& samples,
		       			 const TypeSet<float>& samplesums )
		    : row_( row )
		    , col_( col )
		    , covariancematrix_( covariancematrix )
		    , samples_( samples )
		    , samplesums_( samplesums )
		{}

protected:
    int					nextStep();
    int					row_;
    int 				col_;
    Array2D<float>&			covariancematrix_;
    const ObjectSet<TypeSet<float> >&	samples_;
    const TypeSet<float>&		samplesums_;
};


int PCACovarianceCalculator::nextStep()
{
    const int nrsamples = samples_.size();
    float sum = 0;
    const float rowavg = samplesums_[row_]/nrsamples;
    const float colavg = samplesums_[col_]/nrsamples;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const TypeSet<float>& sample = *(samples_[idx]);
	sum += (sample[row_]-rowavg) *
	       (sample[col_]-colavg);
    }

    const float cov = sum/(nrsamples-1);

    covariancematrix_.set( row_, col_, cov );
    if ( row_!=col_ ) covariancematrix_.set(col_, row_, cov );

    return 0;
}


PCA::PCA( int nrvars )
    : nrvars_( nrvars )
    , covariancematrix_( nrvars, nrvars )
    , samplesums_( nrvars, 0 )
    , eigenvecindexes_( new int[nrvars] )
    , eigenvalues_( new float[nrvars] )
{
    for ( int row=0; row<nrvars_; row++ )
    {
	for ( int col=row; col<nrvars_; col++ )
	{
	    PCACovarianceCalculator* task =
		new PCACovarianceCalculator( covariancematrix_,
					     row, col, samples_, samplesums_ );
	    tasks_ += task;
	    workload_ += Threads::Work( *task, false );
	}
    }
}


PCA::~PCA()
{
    clearAllSamples();
    deepErase( tasks_ );
    delete [] eigenvecindexes_;
    delete [] eigenvalues_;
}


void PCA::clearAllSamples()
{
    deepErase( samples_ );
    samplesums_ = TypeSet<float>( nrvars_, 0 );
}


#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))

bool PCA::tqli( float d[], float e[], int n, ObjectSet<float>& z )
{
    for ( int idx=2;idx<=n;idx++) e[idx-1]=e[idx];

    e[n]=0.0;
    for ( int idx=1; idx<=n; idx++)
    {
	int iter = 0;
	int idy;
	do
	{
	    for ( idy=idx; idy<=n-1; idy++)
	    {
		const float dd = fabs(d[idy])+fabs(d[idy+1]);
		if ( fabs(e[idy])+dd == dd ) break;
	    }

	    if ( idy!=idx )
	    {
		if ( iter++==30)
		{
		    pErrMsg("Too many iterations");
		    return false;
		}

		float g = (d[idx+1]-d[idx])/(2.0f*e[idx]);
		float r = Math::Sqrt((g*g)+1.0f);
		g = d[idy]-d[idx]+e[idx]/(g+SIGN(r,g));

		float c = 1;
		float s = 1;
		float p = 0;
		for ( int idz=idy-1; idz>=idx; idz-- )
		{
		    float f = s*e[idz];
		    const float b = c*e[idz];
		    if ( fabs(f)>=fabs(g) )
		    {
			c= g/f;
			r = Math::Sqrt((c*c)+1.0f);
			e[idz+1] = f*r;
			c *= (s=1.0f/r);
		    }
		    else
		    {
			s = f/g;
			r = Math::Sqrt((s*s)+1.0f);
			e[idz+1] = g*r;
			s *= (c=1.0f/r);
		    }
		    g = d[idz+1]-p;
		    r = (d[idz]-g)*s+2.0f*c*b;
		    p = s*r;
		    d[idz+1] = g+p;
		    g = c*r-b;
		    /* Next loop can be omitted if eigenvectors not wanted */
		    for ( int idu=1; idu<=n; idu++)
		    {
			f = z[idu][idz+1];
			z[idu][idz+1] = s*z[idu][idz]+c*f;
			z[idu][idz] = c*z[idu][idz]-s*f;
		    }
		}
		d[idx] = d[idx]-p;
		e[idx] = g;
		e[idy] = 0.0;
	    }
	} while (idy != idx);
    }

    return true;
}


void PCA::tred2( ObjectSet<float>& a, int n, float d[], float e[])
{
    float f, g;

    for ( int idx=n; idx>=2; idx--)
    {
	const int last = idx-1;
	float scale = 0;
	float h = 0;

	if ( last>1 )
	{
	    for ( int idy=1;idy<=last;idy++)
		scale += fabs(a[idx][idy]);

	    if ( scale==0.0 )
		e[idx] = a[idx][last];
	    else
	    {
		for ( int idy=1; idy<=last; idy++)
		{
		    a[idx][idy] /= scale;
		    h += a[idx][idy]*a[idx][idy];
		}
		f = a[idx][last];
		g = f>0 ? -Math::Sqrt(h) : Math::Sqrt(h);
		e[idx] = scale*g;
		h -= f*g;
		a[idx][last] = f-g;
		f = 0.0;
		for ( int idy=1; idy<=last; idy++)
		{
		    /* Next statement can be omitted if eigenvectors not wanted */
		    a[idy][idx]=a[idx][idy]/h;
		    g=0.0;
		    for ( int idz=1; idz<=idy; idz++)
			g += a[idy][idz]*a[idx][idz];

		    for ( int idz=idy+1;idz<=last;idz++)
			g += a[idz][idy]*a[idx][idz];

		    e[idy]=g/h;
		    f += e[idy]*a[idx][idy];
		}

		const float hh = f/(h+h);
		for ( int idy=1; idy<=last; idy++)
		{
		    f = a[idx][idy];
		    e[idy] = g = e[idy]-hh*f;
		    for ( int idz=1; idz<=idy; idz++)
		    {
			a[idy][idz] -= (f*e[idz]+g*a[idx][idz]);
		    }
		}
	    }
	}
	else
	    e[idx] = a[idx][last];
	d[idx] = h;
    }
    /* Next statement can be omitted if eigenvectors not wanted */
    d[1] = 0.0;
    e[1] = 0.0;
    /* Contents of this loop can be omitted if eigenvectors not
		    wanted except for statement d[i]=a[i][i]; */
    for ( int idx=1; idx<=n; idx++)
    {
	const int last = idx-1;
	if ( d[idx] )
	{
	    for ( int idy=1; idy<=last; idy++)
	    {
		g=0.0;
		for ( int idz=1; idz<=last; idz++)
		{
		    g += a[idx][idz]*a[idz][idy];
		}

		for ( int idz=1; idz<=last; idz++)
		{
		    a[idz][idy] -= g*a[idz][idx];
		}
	    }
	}

	d[idx] = a[idx][idx];
	a[idx][idx] = 1.0;
	for ( int idy=1; idy<=last; idy++)
	{
	    a[idy][idx] = a[idx][idy] = 0.0;
	}
    }
}


bool PCA::calculate()
{
    Threads::WorkManager::twm().addWork( workload_,
	    	Threads::WorkManager::cDefaultQueueID() );

    // Now, get the eigenvalues
    ArrPtrMan<float> d = new float [nrvars_+1];
    ArrPtrMan<float> e = new float [nrvars_+1];
    ObjectSet<float> a;

    float* ptr = covariancematrix_.getData();
    a += ptr;	//Dummy to get counting right
    for ( int idx=0; idx<nrvars_; idx++ )
	a += ptr+idx*nrvars_-1;

    tred2( a, nrvars_, d, e );
    if ( !tqli( d, e, nrvars_, a ) )
	return false;

    for ( int idx=0; idx<nrvars_; idx++ )
    {
	//Store the negative number to get the sorting right
	eigenvalues_[idx] = -d[idx+1];		
	eigenvecindexes_[idx] = idx;
    }

    sort_idxabl_coupled( eigenvalues_, eigenvecindexes_, nrvars_ );

    return true;
}


float PCA::getEigenValue(int idx) const { return -eigenvalues_[idx]; }
