/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: pca.cc,v 1.1 2002-12-30 08:57:35 kristofer Exp $";


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
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	TypeSet<float>& sample = *(samples[idx]);
	sum = sample[row] * sample[col];
    }

    const float cov = (nrsamples*sum - samplesums[row]*samplesums[col]) / 
		      (nrsamples*(nrsamples-1));

    covariancematrix.set( row, col, cov );
    if ( row!=col ) covariancematrix.set(col, row, cov );

    return 0;
}


PCA::PCA( int nrvars_ )
    : nrvars( nrvars_ )
    , covariancematrix( *new Array2DImpl<float>( nrvars_, nrvars_ ) )
    , samplesums( nrvars_, 0 )
    , threadworker( new Threads::ThreadWorkManager(Threads::getNrProcessors()) )
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
    delete threadworker;
    deepErase( tasks );
}


void PCA::clearAllSamples()
{
    deepErase( samples );
    samplesums = TypeSet<float>(nrvars, 0 );
}


void PCA::calculate()
{
    threadworker->addWork( tasks );
    deepErase( tasks );

    // Now, get the eigenvalues
   
    float d[nrvars],e[nrvars];
    ObjectSet<float> a;

    float* ptr = covariancematrix.getData();
    for ( int idx=0; idx<nrvars; idx++ )
	a += ptr+idx*nrvars;

    for (int i=nrvars; i>=1; i--)
    {
	const int l=i-1;
	float h(0), scale(0);
	if ( l )
	{
	    for ( int k=0; k<=l; k++ )
		scale += fabs(a[i][k]);
	    if ( scale==0.0 )
		e[i]=a[i][l];
	    else 
	    {
		for ( int k=0;k<=l;k++)
		{
		    a[i][k] /= scale;
		    h += a[i][k]*a[i][k];
		}

		float f=a[i][l];
		float g = f>0 ? -sqrt(h) : sqrt(h);
		e[i]=scale*g;
		h -= f*g;
		a[i][l]=f-g;
		f=0.0;
		for ( int j=0; j<=l; j++ )
		{
		    a[j][i]=a[i][j]/h;
		    g=0.0;
		    for ( int k=0; k<=j; k++ )
			g += a[j][k]*a[i][k];
		    for ( int k=j+1; k<=l; k++)
			g += a[k][j]*a[i][k];
		    e[j]=g/h;
		    f += e[j]*a[i][j];
		}

		float hh=f/(h+h);
		for ( int j=0; j<=l; j++)
		{
		    f=a[i][j];
		    e[j]=g=e[j]-hh*f;
		    for ( int k=0; k<=j; k++)
			a[j][k] -= (f*e[k]+g*a[i][k]);
		}
	    }
	}
	else
	    e[i]=a[i][l];

	d[i]=h;
    }

    d[1]=0.0;
    e[1]=0.0;

    for ( int i=0; i<nrvars; i++)
    {
	int l=i-1;
	if ( d[i] )
	{
	    for ( int j=0; j<=l; j++)
	    {
		float g=0.0;
		for ( int k=0; k<=l; k++)
		    g += a[i][k]*a[k][j];
		for ( int k=0; k<=l; k++)
		    a[k][j] -= g*a[k][i];
	    }
	}

	d[i]=a[i][i];
	a[i][i]=1.0;

	for (int j=0; j<=l; j++)
	    a[j][i]=a[i][j]=0.0;
    }


#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))
    
    for ( int i=1; i<nrvars; i++) e[i-1]=e[i];
    e[nrvars]=0.0;

    for ( int l=0; l<=nrvars; l++)
    {
	int iter=0;
	int m;
	do
	{
	    for ( m=l; m<nrvars-1; m++)
	    {
		float dd=fabs(d[m])+fabs(d[m+1]);
		if ( fabs(e[m])+dd == dd) break;
	    }
	    if ( m != l)
	    {
		if (iter++ == 30) pErrMsg("Too many iterations");
		float g=(d[l+1]-d[l])/(2.0*e[l]);
		float r=sqrt((g*g)+1.0);
		g=d[m]-d[l]+e[l]/(g+SIGN(r,g));
		float s=1.0,c=1.0;
		float p=0.0;
		for ( int i=m-1; i>=l; i--)
		{
		    float f=s*e[i];
		    float b=c*e[i];
		    if (fabs(f) >= fabs(g))
		    {
			c=g/f;
			r=sqrt((c*c)+1.0);
			e[i+1]=f*r;
			c *= (s=1.0/r);
		    }
		    else
		    {
			s=f/g;
			r=sqrt((s*s)+1.0);
			e[i+1]=g*r;
			s *= (c=1.0/r);
		    }
		    g=d[i+1]-p;
		    r=(d[i]-g)*s+2.0*c*b;
		    p=s*r;
		    d[i+1]=g+p;
		    g=c*r-b;

		    for ( int k=0; k<nrvars; k++)
		    {
			f=a[k][i+1];
			a[k][i+1]=s*a[k][i]+c*f;
			a[k][i]=c*a[k][i]-s*f;
		    }
		}

		d[l]=d[l]-p;
		e[l]=g;
		e[m]=0.0;
	    }
	} while (m != l);
    }
}
