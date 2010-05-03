#ifndef wavelettrans_h
#define wavelettrans_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: wavelettrans.h,v 1.18 2010-05-03 15:11:44 cvsyuancheng Exp $
________________________________________________________________________

@$*/

#include "transform.h"
#include "enums.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "fft.h"
#include "ranges.h"

/*!\brief
WaveletTransform is a ND wavelet transform.
\par
Specify wavelet at creation, and use in the same way as any TransformND.
The algorithm is based on the one from NumericalRecipies, and additional 
kernel support comes from the Matlab library "WaveLab" (Stanford University).
*/

mClass WaveletTransform
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

			DeclareEnumUtils(WaveletType);

    static void		getInfo(WaveletType tp,int& len,TypeSet<float>&);

    static const float 	haar[3];

    static const float 	daub4[5];
    static const float 	daub6[7];
    static const float 	daub8[9];
    static const float 	daub10[11];
    static const float 	daub12[13];
    static const float 	daub14[15];
    static const float 	daub16[17];
    static const float 	daub18[19];
    static const float 	daub20[21];

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


mClass DWT : public GenericTransformND
{
public:
			DWT( WaveletTransform::WaveletType );

    bool		real2real() const;
    bool		real2complex() const		{ return false; }
    bool		complex2real() const		{ return false; }
    bool		complex2complex() const		{ return true; }

    bool		biDirectional( ) const		{ return true; };

    bool		init();

protected:

    mClass FilterWT1D : public GenericTransformND::Transform1D
    {
    public:
	
	void		setSize(int nsz) { size=nsz; }
	int		getSize() const { return size; }
	void		setDir(bool nf) { forward=nf; }
	bool		getDir() const { return forward; }

	bool		init();

	void		transform1D( const float_complex*, float_complex*,
				     int space) const;
	void		transform1D( const float*, float*, int space) const;

			FilterWT1D()
			    : size (-1)
			    , cc( 0 )
			    , cr( 0 )
			    , forward( true )
			    , wt ( WaveletTransform::Haar )
			{}	

			~FilterWT1D() { delete cr; delete cc; }

	void		setWaveletType( WaveletTransform::WaveletType );
    protected:

#include <templ_wavlttransimpl.h>

	WaveletTransform::WaveletType		wt;
	int			size;
	bool			forward;

	float*			cc;		// Filter Parameters
	float*			cr;
	int			filtersz;
	int			joff;
	int			ioff;
    };

    Transform1D*		createTransform() const
				{ return new FilterWT1D; }

    bool			isPossible( int ) const;
    bool			isFast( int ) const { return true; };


    WaveletTransform::WaveletType	wt;
};


mClass CWT : public TransformND
{
public:
			CWT();
			~CWT();

    enum		WaveletType { Morlet, Gaussian, MexicanHat };
    			DeclareEnumUtils(WaveletType);

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

    bool		init();

    bool		transform(const ArrayND<float>&,
				   ArrayND<float>& ) const
			{ return false; }
    bool		transform(const ArrayND<float_complex>&,
				   ArrayND<float_complex>& ) const
			{ return false; }
    bool		transform(const ArrayND<float_complex>& input,
	    				ArrayND<float>& output) const;

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
				  Array2DImpl<float>&) const;

    FFT			fft_;
    FFT			ifft_;
	    			     
    ArrayNDInfo*	info_;

    bool		inited;
    float		dt_;
    WaveletType		wt_;

    StepInterval<float> freqrg_;
    TypeSet<int>	outfreqidxs_;
};

#endif
