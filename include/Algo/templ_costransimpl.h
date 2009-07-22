/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          2001
 RCS:           $Id: templ_costransimpl.h,v 1.4 2009-07-22 16:01:12 cvsbert Exp $
________________________________________________________________________

 This is part of another include file. Do not protect against multiple
 inclusion.

-*/

#define invroot2 0.7071067814

template<class T> inline
void inv_sums(T* signal, int space) const
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
void fwd_sums(T* signal, int space) const
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
void scramble( T* signal, int space ) const
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
void unscramble(T* signal, int space ) const
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
void inv_butterflies( T* signal, int space) const
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
void fwd_butterflies(T* signal, int space ) const
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
void bitrev( T* in, int space, int len ) const
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
void templ_transform1D( const T* in, T* out, int space ) const
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
	    scramble( out, space );
	    fwd_butterflies( out, space );
	    bitrev( out, space, size );
	    fwd_sums( out, space );
	}
	else
	{
	    out[0] *= invroot2;
	    inv_sums( out, space );
	    bitrev( out, space, size );
	    inv_butterflies( out, space );
	    unscramble( out, space );

	    int end = size*space;
	    for ( int idx=0; idx<end; idx+=space )
		out[idx] *= two_over_size;
	}

	if ( forward ) out[0] *= invroot2;
    }
    else
    {
	ArrPtrMan<T> tmp = new T [size];
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
		double factor = idf*M_PI/2.0/size;

		for ( int idx=0; idx<size; idx++ )
		{
		    T toadd = indata[idx*inspace];
		    toadd *= ((double)(cos( (2*idx+1) * factor )));
		    sum += toadd;
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
		    T contrib = indata[idf*inspace];
		    contrib *= (idf?two_over_size:two_over_size*invroot2) *
				cos((2*idx+1)*idf*factor);
		    sum += contrib;
		}

		out[idx*space] = sum;
	    }
	}

	if ( forward ) out[0] *= invroot2;
    }
}
