#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "transform.h"
#include "enums.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "ranges.h"

/*!
\brief WaveletTransform is a ND wavelet transform.

  \par
  Specify wavelet at creation, and use in the same way as any TransformND.
  The algorithm is based on the one from NumericalRecipies, and additional
  kernel support comes from the Matlab library "WaveLab" (Stanford University).
*/

mExpClass(Algo) WaveletTransform
{
public:
    enum		WaveletType { Haar, Daubechies4, Daubechies6,
					Daubechies8, Daubechies10,
					Daubechies12, Daubechies14,
					Daubechies16, Daubechies18,
					Daubechies20, Beylkin, Coiflet1,
					Coiflet2, Coiflet3, Coiflet4,
					Coiflet5, Symmlet4, Symmlet5,
					Symmlet6, Symmlet7, Symmlet8,
					Symmlet9, Symmlet10, Vaidyanathan };

			mDeclareEnumUtils(WaveletType);

    static void		getInfo(WaveletType tp,int& len,TypeSet<float>&);

    static const float	haar[3];

    static const float	daub4[5];
    static const float	daub6[7];
    static const float	daub8[9];
    static const float	daub10[11];
    static const float	daub12[13];
    static const float	daub14[15];
    static const float	daub16[17];
    static const float	daub18[19];
    static const float	daub20[21];

    static const float	beylkin[19];

    static const float	coiflet1[7];
    static const float	coiflet2[13];
    static const float	coiflet3[19];
    static const float	coiflet4[25];
    static const float	coiflet5[31];

    static const float	symmlet4[9];
    static const float	symmlet5[11];
    static const float	symmlet6[13];
    static const float	symmlet7[15];
    static const float	symmlet8[17];
    static const float	symmlet9[19];
    static const float	symmlet10[21];

    static const float	vaidyanathan[25];

    static bool		isCplx( WaveletType );
};


/*!
\brief Discrete Wavelet Transform
*/

mExpClass(Algo) DWT : public GenericTransformND
{
public:
			DWT( WaveletTransform::WaveletType );
    bool		setup() override;

protected:

    mExpClass(Algo) FilterWT1D : public GenericTransformND::Transform1D
    {
    public:

	bool		init() override;
	bool		run(bool) override;
			FilterWT1D()
			    : cc_( 0 )
			    , cr_( 0 )
			    , wt_( WaveletTransform::Haar )
			{}

			~FilterWT1D() { delete [] cr_; delete [] cc_; }

	void		setWaveletType( WaveletTransform::WaveletType );
    protected:

	template <class T> inline
	void transform1Dt( const T* in, T* out, int space ) const;

	WaveletTransform::WaveletType		wt_;

	float*			cc_;		// Filter Parameters
	float*			cr_;
	int			filtersz_;
	int			joff_;
	int			ioff_;
    };

    Transform1D*		createTransform() const override
				{ return new FilterWT1D; }

    WaveletTransform::WaveletType	wt_;
};


template <class T> inline
void DWT::FilterWT1D::transform1Dt( const T* in, T* out, int space ) const
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
	    mAllocLargeVarLenArr( T, wksp, nn );
	    OD::sysMemZero( wksp, sizeof(T)*nn );
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
	    mAllocLargeVarLenArr( T, wksp, nn );
	    OD::sysMemZero( wksp, sizeof(T)*nn );
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


/*!
\brief Continuous Wavelet Transform
*/

mExpClass(Algo) CWT
{
public:
			CWT();
			~CWT();

    bool		init();


    enum		WaveletType { Morlet, Gaussian, MexicanHat };
			mDeclareEnumUtils(WaveletType);

    void		setWavelet(CWT::WaveletType);

    void		setTransformRange( const StepInterval<float>& rg )
			{ freqrg_ = rg; }
    void		setDeltaT( float dt )		{ dt_ = dt; }

    bool		setInputInfo(const ArrayNDInfo&);
    const ArrayNDInfo&	getInputInfo() const		{ return *info_; }

    bool		real2real() const		{ return true; }
    bool		real2complex() const		{ return false; }
    bool		complex2real() const		{ return false; }
    bool		complex2complex() const		{ return true; }

    bool		biDirectional() const		{ return false; }
    bool		setDir(bool forw);
    bool		getDir() const			{ return true; }


    bool		transform(const ArrayND<float>&,
				   ArrayND<float>& ) const
			{ return false; }
    bool		transform(const ArrayND<float_complex>&,
				   ArrayND<float_complex>& ) const
			{ return false; }
    bool		transform(const ArrayND<float_complex>& input,
					ArrayND<float>& output);

    float		getScale(int ns,float dt,float freq) const;

    void		setFreqIdxs( const TypeSet<int>& outfreqidxs )
			{ outfreqidxs_ = outfreqidxs; }

protected:

    struct CWTWavelets
    {
				CWTWavelets()	{}

	void			createWavelet(WaveletType,int nrsamples,
					      float scale);
	const TypeSet<float>*	getWavelet(float scale) const;
	void			createMorletWavelet(int,float,TypeSet<float>&);
	void			createMexhatWavelet(int,float,TypeSet<float>&);
	void			createGaussWavelet(int,float,TypeSet<float>&);


	TypeSet<float>		scales_;
	TypeSet< TypeSet<float> > wavelets_;
    };


    CWTWavelets		wvlts_;

    bool		isPossible(int sz) const;
    bool		isFast( int ) const { return true; }

    void		transform(int,float,int,
				  const Array1DImpl<float_complex>&,
				  Array2DImpl<float>&);

    Fourier::CC*	fft_;
    Fourier::CC*	ifft_;

    ArrayNDInfo*	info_;

    bool		inited_;
    float		dt_;
    WaveletType		wt_;

    StepInterval<float> freqrg_;
    TypeSet<int>	outfreqidxs_;
};
