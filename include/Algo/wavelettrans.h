#ifndef wavelettrans_h
#define wavelettrans_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: wavelettrans.h,v 1.4 2001-07-23 14:30:00 kristofer Exp $
________________________________________________________________________

@$*/

#include <transform.h>
#include <enums.h>
#include <simpnumer.h>
#include <arraynd.h>
#include <ptrman.h>

/*!\brief
WaveletTransform is a ND wavelet transform.
\par
Specify wavelet at creation, and use in the same way as any TransformND.
The algorithm is taken from NumericalRecipies, and additional kernel support
comes from the Matlab library "WaveLab" (Stanford University).
*/

class WaveletTransform
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


class DiscreteWaveletTransform : public GenericTransformND
{
    isConcreteClass
public:
		    DiscreteWaveletTransform( WaveletTransform::WaveletType );

    bool		isReal() const;
    bool		isCplx() const { return true; }

    bool		bidirectional( ) const { return true; };

    bool		init();

protected:

    class FilterWT1D : public GenericTransformND::Transform1D
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


class ContiniousWaveletTransform : public TransformND
{
    isConcreteClass
public:
		    ContiniousWaveletTransform( WaveletTransform::WaveletType );
		    ~ContiniousWaveletTransform();


    bool		setInputInfo( const ArrayNDInfo& );
    const ArrayNDInfo&	getInputInfo() const { return *inputinfo; }
    const ArrayNDInfo&	getOutputInfo() const { return *outputinfo; }

    bool		isReal() const;
    bool		isCplx() const { return true; }

    bool		bidirectional() const { return false; }
    bool		setDir( bool forward ) { return forward; }
    bool		getDir() const { return true; }

    bool		init();

    bool		transform(const ArrayND<float>&,
				   ArrayND<float>& ) const;
    bool		transform(const ArrayND<float_complex>&,
				   ArrayND<float_complex>& ) const;

protected:
    bool		isPossible( int ) const;
    bool		isFast( int ) const { return true; }

    void		transform1D( const ArrayND<float>::LinearStorage&,
				     ArrayND<float>::LinearStorage&,
				     int insz, int inpoff, int inpspace,
				     int outpoff, int outpspace ) const;

    void		transformOneDim( const ArrayND<float>&,
	    			     ArrayND<float>&, int dim ) const;
	    			     


    template <class T> class Wavelet
    {
    public:
		Wavelet( WaveletTransform::WaveletType, float scale );
		~Wavelet();
	float	correllate( const ArrayND<float>::LinearStorage&,
			    int size, int off, int space ) const;
	float	correllate( const ArrayND<float_complex>::LinearStorage*,
			    int off, int space ) const;

    protected:
	float*	data;
	int	len;
	int	firstpos;
    };

    ObjectSet< Wavelet<float> >		realwavelets;

    ArrayNDInfo*			inputinfo;
    ArrayNDInfo*			outputinfo;

    WaveletTransform::WaveletType 	wt;
};


#endif
