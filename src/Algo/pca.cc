/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: pca.cc,v 1.5 2003-11-07 12:21:57 bert Exp $";


#include "pca.h"

#include "arrayndimpl.h"
#include "basictask.h"
#include "errh.h"
#include "thread.h"
#include "threadwork.h"


class PCACovarianceCalculator : public BasicTask
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
	TypeSet<float>& sample = *(samples[idx]);
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
    , covariancematrix( *new Array2DImpl<float>( nrvars_, nrvars_ ) )
    , samplesums( nrvars_, 0 )
    , threadworker( 0 )
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
    delete &covariancematrix;
    deepErase( tasks );
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
		float r = sqrt((g*g)+1.0);
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
			r = sqrt((c*c)+1.0);
			e[idz+1] = f*r;
			c *= (s=1.0/r);
		    }
		    else
		    {
			s = f/g;
			r = sqrt((s*s)+1.0);
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
		g = f>0 ? -sqrt(h) : sqrt(h);
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
	{
	    while ( tasks[idx]->doStep()>0 )
		;
	}
    }


    // Now, get the eigenvalues
   
    float d[nrvars+1],e[nrvars+1];
    ObjectSet<float> a;

    float* ptr = covariancematrix.getData();
    a += ptr;	//Dummy to get counting right
    for ( int idx=0; idx<nrvars; idx++ )
	a += ptr+idx*nrvars-1;

    tred2( a, nrvars, d, e );
    if ( !tqli( d, e, nrvars, a ) )
	return false;

    eigenvalues.erase();

    for ( int idx=1; idx<=nrvars; idx++ )
	eigenvalues += d[idx];

    return true;
}


float PCA::getEigenValue(int idx) const { return eigenvalues[idx]; }


void PCA::getEigenVector(int idy, TypeSet<float>& res ) const
{
    res.erase();
    for ( int idx=0; idx<nrvars; idx++ )
	res += covariancematrix.get(idx, idy );
}


void PCA::setThreadWorker( Threads::ThreadWorkManager* nv )
{
    threadworker = nv;
}
