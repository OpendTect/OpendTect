#ifndef wavelettrans_h
#define wavelettrans_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: wavelettrans.h,v 1.2 2001-06-02 12:40:53 windev Exp $
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


class WaveletTransform : public GenericTransformND
{
    isConcreteClass
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

			WaveletTransform( WaveletType );

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
			{}	

			~FilterWT1D() { delete cr; delete cc; }

	void		setWaveletType( WaveletType );
    protected:

#include <templ_wavlttransimpl.h>

	WaveletType		wt;
	int			size;
	bool			forward;

	float*			cc;		// Filter Parameters
	float*			cr;
	int			filtersz;
	int			joff;
	int			ioff;


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
    };

    Transform1D*		createTransform() const
				{ return new FilterWT1D; }

    bool			isPossible( int ) const;
    bool			isFast( int ) const { return true; };

    static bool			isCplx( WaveletType );

    WaveletType			wt;

};


#endif
