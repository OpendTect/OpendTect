/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          2001
 RCS:           $Id: templ_wavlttransimpl.h,v 1.5 2010/08/11 16:55:33 cvsyuancheng Exp $
________________________________________________________________________

 This is part of another include file. Do not protect against multiple
 inclusion.

-*/

template <class T> inline
void transform1Dt( const T* in, T* out, int space ) const
{
    if ( in != out )
    {
	int end = sz_ * space;
	
	for ( int idx=0; idx<end; idx+=space )
	    out[idx] = in[idx];
    }

    if ( forward_ )
    {
	for ( int nn=sz_; nn>=2; nn>>=1 )
	{
	    ArrPtrMan<T> wksp = new T [nn];
	    memset( wksp, 0, sizeof(T)*nn );
	    int nmod = nn*filtersz_;
	    int n1 = nn-1;
	    int nh = nn >> 1;

	    int i = 1;
	    for ( int ii=0; i<=nn; i+=2, ii++ )
	    {
		int ni=i+nmod+ioff_;
		int nj=i+nmod+joff_;

		for ( int k=1; k<=filtersz_; k++ )
		{
		    int jf = n1 & (ni+k);
		    int jr = n1 & (nj+k);

		    wksp[ii] += cc_[k]*out[jf*space];
		    wksp[ii+nh] += cr_[k]*out[jr*space];
		}
	    }

	    for ( int j=0; j<nn; j++ )
		out[j*space] = wksp[j];
	}
    }
    else
    {
	for ( int nn=2; nn<=sz_; nn<<=1 )
	{
	    ArrPtrMan<T> wksp = new T [nn];
	    memset( wksp, 0, sizeof(T)*nn );
	    int nmod = nn*filtersz_;
	    int n1 = nn-1;
	    int nh = nn >> 1;

	    int i = 1;
	    for ( int ii=0; i<nn; i+=2, ii++ )
	    {
		T ai=out[ii*space];
		T ai1=out[(ii+nh)*space];
		int ni =i+nmod+ioff_;
		int nj =i+nmod+joff_;

		for (int k=1; k<=filtersz_; k++ )
		{
		    int jf = (n1 & (ni+k));
		    int jr = (n1 & (nj+k));
	       
		    wksp[jf] += cc_[k]*ai; 
		    wksp[jr] += cr_[k]*ai1; 
		}
	    }

	    for ( int j=0; j<nn; j++ )
		out[j*space] = wksp[j];
	}
    }
}
