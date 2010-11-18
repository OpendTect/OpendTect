/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: pca.cc,v 1.14 2010-11-18 17:48:14 cvskris Exp $";


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
    		PCACovarianceCalculator( Array2D<float>& covariancematrix_,
					 int row_, int col_,
					 ObjectSet<TypeSet<float> >& samples_,
		       			 const TypeSet<float>& samplesums_ )
		    : row( row_ )
		    , col( col_ )
		    , covariancematrix( covariancematrix_ )
		    , samples( samples_ )
		    , samplesums( samplesums_ )
		{}

protected:
    int					nextStep();
    int					row;
    int 				col;
    Array2D<float>&			covariancematrix;
    const ObjectSet<TypeSet<float> >&	samples;
    const TypeSet<float>&		samplesums;
};


int PCACovarianceCalculator::nextStep()
{
    const int nrsamples = samples.size();
    float sum = 0;
    const float rowavg = samplesums[row]/nrsamples;
    const float colavg = samplesums[col]/nrsamples;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const TypeSet<float>& sample = *(samples[idx]);
	sum += (sample[row]-rowavg) *
	       (sample[col]-colavg);
    }

    const float cov = sum/(nrsamples-1);

    covariancematrix.set( row, col, cov );
    if ( row!=col ) covariancematrix.set(col, row, cov );

    return 0;
}


PCA::PCA( int nrvars_ )
    : nrvars( nrvars_ )
    , covariancematrix( nrvars_, nrvars_ )
    , samplesums( nrvars_, 0 )
    , threadworker( 0 )
    , eigenvecindexes( new int[nrvars_] )
    , eigenvalues( new float[nrvars_] )
{
    for ( int row=0; row<nrvars; row++ )
    {
	for ( int col=row; col<nrvars; col++ )
	{
	    tasks += new PCACovarianceCalculator( covariancematrix,
				    row, col, samples, samplesums );
	}
    }
}


PCA::~PCA()
{
    clearAllSamples();
    deepErase( tasks );
    delete [] eigenvecindexes;
    delete [] eigenvalues;
}


void PCA::clearAllSamples()
{
    deepErase( samples );
    samplesums = TypeSet<float>(nrvars, 0 );
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

		float g = (d[idx+1]-d[idx])/(2.0*e[idx]);
		float r = Math::Sqrt((g*g)+1.0);
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
			r = Math::Sqrt((c*c)+1.0);
			e[idz+1] = f*r;
			c *= (s=1.0/r);
		    }
		    else
		    {
			s = f/g;
			r = Math::Sqrt((s*s)+1.0);
			e[idz+1] = g*r;
			s *= (c=1.0/r);
		    }
		    g = d[idz+1]-p;
		    r = (d[idz]-g)*s+2.0*c*b;
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
    if ( threadworker ) threadworker->addWork( tasks );
    else
    {
	const int nrtasks=tasks.size();
	for ( int idx=0; idx<nrtasks; idx++ )
	    tasks[idx]->execute();
    }

    // Now, get the eigenvalues
    ArrPtrMan<float> d = new float [nrvars+1];
    ArrPtrMan<float> e = new float [nrvars+1];
    ObjectSet<float> a;

    float* ptr = covariancematrix.getData();
    a += ptr;	//Dummy to get counting right
    for ( int idx=0; idx<nrvars; idx++ )
	a += ptr+idx*nrvars-1;

    tred2( a, nrvars, d, e );
    if ( !tqli( d, e, nrvars, a ) )
	return false;

    for ( int idx=0; idx<nrvars; idx++ )
    {
	//Store the negative number to get the sorting right
	eigenvalues[idx] = -d[idx+1];		
	eigenvecindexes[idx] = idx;
    }

    sort_idxabl_coupled( eigenvalues, eigenvecindexes, nrvars );

    return true;
}


float PCA::getEigenValue(int idx) const { return -eigenvalues[idx]; }


void PCA::setThreadWorker( Threads::WorkManager* nv )
{
    threadworker = nv;
}
