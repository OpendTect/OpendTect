/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: costrans.cc,v 1.1 2001-02-19 17:17:02 bert Exp $";


#include <costrans.h>
#include <simpnumer.h>
#include <arraynd.h>

DefineClassID(CosineTransform);


bool CosineTransform::CosineTransform1D::init()
{
    if ( !isPower( size, 2 ) )
    {
	isfast = false;
    }
    else
    {
	isfast = true;
	initcosarray();
    }

    two_over_size = 2.0/(double)size;
    root2_over_rootsize = sqrt(2.0/(double)size);

    return true;
}


#define invroot2 0.7071067814

template<class T> inline
void CosineTransform::CosineTransform1D::inv_sums(T* signal, int space) const
{
    for ( int stage=1; stage<power; stage++ )
    {
	int nthreads = 1 << (stage-1);
	int stepsize = nthreads << 1;
	int nsteps   = (1 << (power-stage)) - 1;

	for ( int thread=1; thread<=nthreads; thread++ )
	{
	    int curptr=size-thread; 

	    for ( int step=1; step<=nsteps; step++ )
	    {
		signal[curptr*space] += signal[(curptr-stepsize)*space];
		curptr -= stepsize; 
	    }
	}
    }
}


template<class T> inline
void CosineTransform::CosineTransform1D::fwd_sums(T* signal, int space) const
{
    for ( int stage=power-1; stage>=1; stage-- )
    {
	int nthreads = 1 << (stage-1);
	int stepsize = nthreads << 1;
	int nsteps   = (1 << (power-stage)) - 1;

	for ( int thread=1; thread<=nthreads; thread++)
	{
	    int curptr = nthreads+thread-1;

	    for ( int step=1; step<=nsteps; step++)
	    {
		signal[curptr*space] += signal[(curptr+stepsize)*space];
		curptr += stepsize;
	    }
	}
    }
}


template<class T> inline
void CosineTransform::CosineTransform1D::scramble( T* signal, int space ) const
{
    int hsz = size >> 1;
    int qtrsz = hsz >> 1;

    bitrev( signal, space, size );
    bitrev( signal, space, hsz );
    bitrev( &signal[hsz*space], space, hsz );

    int ii1 = (size-1)*space;
    int ii2 = hsz*space;

    for ( int i=0; i<qtrsz; i++ )
    {
	T temp = signal[ii1];
	signal[ii1] = signal[ii2];
	signal[ii2] = temp;
	ii1 -= space;
	ii2 += space;
    }
}

template<class T> inline
void CosineTransform::CosineTransform1D::unscramble(T* signal, int space ) const
{
    int hsz = size >> 1;
    int qtrsz = hsz >> 1;
    int ii1 = (size-1)*space;
    int ii2 = hsz*space;

    for ( int i=0; i<qtrsz; i++ )
    {
	T tmp = signal[ii1];
	signal[ii1] = signal[ii2];
	signal[ii2] = tmp;
	ii1 -= space;
	ii2 += space;
    }

    bitrev( signal, space, hsz);
    bitrev( &signal[hsz*space], space, hsz);
    bitrev( signal, space, size);
}


template<class T> inline
void CosineTransform::CosineTransform1D::inv_butterflies( T* signal,
							  int space) const
{
    for ( int stage=1; stage<=power; stage++ )
    {
	int ngroups = 1 << (power-stage);
	int wingspan = 1 << (stage-1);
	int increment = wingspan << 1;

	for ( int butterfly=1; butterfly<=wingspan; butterfly++ )
	{
	    T Cfac = cosarray[wingspan+butterfly-1];
	    int baseptr=0;

	    for ( int group=1; group<=ngroups; group++ )
	    {
		int i1 = baseptr+butterfly-1;
		int ii1 = i1*space;
		int ii2 = (i1+wingspan)*space;
		T tmp = Cfac*signal[ii2];
		signal[ii2] = signal[ii1]-tmp;
		signal[ii1] = signal[ii1]+tmp;
		baseptr += increment;
	    }
	}
    }
}

template<class T> inline
void CosineTransform::CosineTransform1D::fwd_butterflies(T* signal,
							 int space ) const
{
    for ( int stage=power; stage>=1; stage-- )
    {
	int ngroups = 1 << (power-stage);
	int wingspan = 1 << (stage-1);
	int increment = wingspan << 1;

	for ( int butterfly=1; butterfly<=wingspan; butterfly++ )
	{
	    T Cfac = cosarray[wingspan+butterfly-1];
	    int baseptr = 0;

	    for ( int group=1; group<=ngroups; group++ )
	    {
		int i1 = baseptr+butterfly-1;
		int ii1 = i1*space;
		int ii2 = (i1+wingspan)*space;
		T tmp = signal[ii2];
		signal[ii2] = Cfac * (signal[ii1]-tmp);
		signal[ii1] = signal[ii1]+tmp;
		baseptr += increment;
	    }
	}
    }
}


template<class T> inline
void CosineTransform::CosineTransform1D::bitrev( T* in, int space,
						 int len ) const
{
    if ( len<=2 ) return; /* No action necessary if n=1 or n=2 */
    int hsz = len>>1;

    int j=1;
    for ( int i=1; i<=len; i++ )
    {
	if( i<j )
	{
	    int offi = space*(i-1);
	    int offj = space*(j-1);

	    T temp = in[offj];
	    in[offj] = in[offi];
	    in[offi] = temp;
	}

	int m = hsz;

	while ( j>m )
	{
	    j = j-m;
	    m= (m+1) >> 1;
    	}

	j=j+m;
    }
}


template <class T> inline
void CosineTransform::CosineTransform1D::templ_transform1D( const T* in,
							    T* out,
							    int space ) const
{
    if ( isfast )
    {
	if ( in != out )
	{
	    int end = size*space;

	    for ( int idx=0; idx<end; idx+=space )
		out[idx] = in[idx];
	}

	if ( forward )
	{
	    scramble<T>( out, space );
	    fwd_butterflies<T>( out, space );
	    bitrev<T>( out, space, size );
	    fwd_sums<T>( out, space );
	}
	else
	{
	    out[0] *= invroot2;
	    inv_sums<T>( out, space );
	    bitrev<T>( out, space, size );
	    inv_butterflies<T>( out, space );
	    unscramble<T>( out, space );

	    int end = size*space;
	    for ( int idx=0; idx<end; idx+=space )
		out[idx] *= two_over_size;
	}

	if ( forward ) out[0] *= invroot2;
    }
    else
    {
	T tmp[size];
	const T* indata = in;
	int inspace = space;

	if ( in == out )
	{
	    for ( int idx=0; idx<size; idx++ )
		tmp[idx] = in[idx*space];

	    indata = tmp;
	    inspace = 1;
	}

	if ( forward )
	{
	    for ( int idf=0; idf<size; idf++ )
	    {
		T sum = 0;
		double factor = idf*M_PI/2.0/size;;

		for ( int idx=0; idx<size; idx++ )
		{
		    sum += indata[idx*inspace]*cos((2*idx+1)*factor);
		}

		out[idf*space] = sum;
	    }
	}
	else
	{
	    double factor = M_PI/2/size;

	    for ( int idx=0; idx<size; idx++ )
	    {
		T sum = 0;

		for ( int idf=0; idf<size; idf++ )
		{
		    T contrib = indata[idf*inspace] *
				(idf?two_over_size:two_over_size*invroot2) *
				cos((2*idx+1)*idf*factor);
		    sum += contrib;
		}

		out[idx*space] = sum;
	    }
	}
	

	if ( forward ) out[0] *= invroot2;
    }
}


void CosineTransform::CosineTransform1D::initcosarray()
{
    power = isPower( size, 2 );

    if( cosarray ) delete cosarray;

    cosarray = new float[size];

    int hsz=size/2;

    for ( int i=0; i<=hsz-1; i++ ) cosarray[hsz+i]=4*i+1;

    for ( int group=1; group<=power-1; group++ )
    {
	int base= 1 << (group-1);
	int nitems = base;
	float factor = 1.0*(1<<(power-group));

	for ( int item=1; item<=nitems; item++ )
	{
	    cosarray[base+item-1] = factor*cosarray[hsz+item-1];
	}
    }

    for ( int i=1; i<size; i++ )
	cosarray[i] = 1.0/(2.0*cos(cosarray[i]*M_PI/(2.0*size)));
}

void CosineTransform::CosineTransform1D::transform1D( const float_complex* in,
						      float_complex* out,
						      int space ) const
{
    templ_transform1D<float_complex>( in, out, space );
}


void CosineTransform::CosineTransform1D::transform1D( const float* in,
						      float* out,
						      int space ) const
{
    templ_transform1D<float>( in, out, space );
}


bool CosineTransform::isPossible( int sz ) const
{
    return true;
}


bool CosineTransform::isFast( int sz ) const
{
    return isPower( sz, 2 );
}
