/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          2001
 RCS:           $Id: templ_costransimpl.h,v 1.5 2010/08/11 16:55:33 cvsyuancheng Exp $
________________________________________________________________________

 This is part of another include file. Do not protect against multiple
 inclusion.

-*/

#define invroot2 0.7071067814

template<class T> inline
void inv_sums(T* signal, int space) const
{
    for ( int stage=1; stage<power_; stage++ )
    {
	int nthreads = 1 << (stage-1);
	int stepsize = nthreads << 1;
	int nsteps   = (1 << (power_-stage)) - 1;

	for ( int thread=1; thread<=nthreads; thread++ )
	{
	    int curptr=sz_-thread; 

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
    for ( int stage=power_-1; stage>=1; stage-- )
    {
	int nthreads = 1 << (stage-1);
	int stepsize = nthreads << 1;
	int nsteps   = (1 << (power_-stage)) - 1;

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
    int hsz = sz_ >> 1;
    int qtrsz = hsz >> 1;

    bitrev( signal, space, sz_ );
    bitrev( signal, space, hsz );
    bitrev( &signal[hsz*space], space, hsz );

    int ii1 = (sz_-1)*space;
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
    int hsz = sz_ >> 1;
    int qtrsz = hsz >> 1;
    int ii1 = (sz_-1)*space;
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
    bitrev( signal, space, sz_);
}


template<class T> inline
void inv_butterflies( T* signal, int space) const
{
    for ( int stage=1; stage<=power_; stage++ )
    {
	int ngroups = 1 << (power_-stage);
	int wingspan = 1 << (stage-1);
	int increment = wingspan << 1;

	for ( int butterfly=1; butterfly<=wingspan; butterfly++ )
	{
	    T Cfac = cosarray_[wingspan+butterfly-1];
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
    for ( int stage=power_; stage>=1; stage-- )
    {
	int ngroups = 1 << (power_-stage);
	int wingspan = 1 << (stage-1);
	int increment = wingspan << 1;

	for ( int butterfly=1; butterfly<=wingspan; butterfly++ )
	{
	    T Cfac = cosarray_[wingspan+butterfly-1];
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
    if ( isfast_ )
    {
	if ( in != out )
	{
	    int end = sz_*space;

	    for ( int idx=0; idx<end; idx+=space )
		out[idx] = in[idx];
	}

	if ( forward_ )
	{
	    scramble( out, space );
	    fwd_butterflies( out, space );
	    bitrev( out, space, sz_ );
	    fwd_sums( out, space );
	}
	else
	{
	    out[0] *= invroot2;
	    inv_sums( out, space );
	    bitrev( out, space, sz_ );
	    inv_butterflies( out, space );
	    unscramble( out, space );

	    int end = sz_*space;
	    for ( int idx=0; idx<end; idx+=space )
		out[idx] *= two_over_size_;
	}

	if ( forward_ ) out[0] *= invroot2;
    }
    else
    {
	ArrPtrMan<T> tmp = new T [sz_];
	const T* indata = in;
	int inspace = space;

	if ( in == out )
	{
	    for ( int idx=0; idx<sz_; idx++ )
		tmp[idx] = in[idx*space];

	    indata = tmp;
	    inspace = 1;
	}

	if ( forward_ )
	{
	    for ( int idf=0; idf<sz_; idf++ )
	    {
		T sum = 0;
		double factor = idf*M_PI/2.0/sz_;

		for ( int idx=0; idx<sz_; idx++ )
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
	    double factor = M_PI/2/sz_;

	    for ( int idx=0; idx<sz_; idx++ )
	    {
		T sum = 0;

		for ( int idf=0; idf<sz_; idf++ )
		{
		    T contrib = indata[idf*inspace];
		    contrib *= (idf?two_over_size_:two_over_size_*invroot2) *
				cos((2*idx+1)*idf*factor);
		    sum += contrib;
		}

		out[idx*space] = sum;
	    }
	}

	if ( forward_ ) out[0] *= invroot2;
    }
}
