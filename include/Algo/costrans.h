#ifndef costrans_h
#define costrans_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: costrans.h,v 1.10 2010-05-03 15:11:44 cvsyuancheng Exp $
________________________________________________________________________


@$*/

#include "transform.h"
#include "ptrman.h"

/*!\brief
This is the CosineTransform.
\par
It is fast when the size is a power of two, slow otherwise.
The fast transform is based on the article:
Sherlock, Barry G. & Monro, Donald M., 1995: Algorithm 749: Fast discrete cosine
transform.  ACM Transactions on Mathematical Software. Volume 21(4).
Pages 372-378.

The slow transform is based on:
Gonzales, Rafael C. & Woods, Richard E., 1992: Digital Image Processing.
Addison-Wesley Publishing Company. Pages 143-144.

The cosine transform is a transform that is similar to the FourierTransform,
with the differences that:
1. It is real.
2. It does not assume that the signal repeats itself, resulting in that no
windowing is required prior to the transform.

The CosineTransorm is mostly known from the JPEG (.jpg, .mp3 ++) compression
standard, where it is used extensively.

*/


mClass CosineTransform : public GenericTransformND
{
public:
    bool		isReal(bool) const { return true; }
    bool		isCplx(bool) const { return true; }

    bool		biDirectional() const { return true; }

protected:

    mClass CosineTransform1D : public GenericTransformND::Transform1D
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

			CosineTransform1D()
			    : size (-1)
			    , cosarray( 0 )
			    , forward( true )
			{}	

			~CosineTransform1D() { delete cosarray; }
    protected:

	float*		cosarray;
	int		size;
	bool		forward;
	int 		power;
	bool		isfast;

	float		two_over_size;
	float		root2_over_rootsize;

#include <templ_costransimpl.h>


	void				initcosarray();
    };

    Transform1D*			createTransform() const
					{ return new CosineTransform1D; }

    bool				isPossible( int ) const;
    bool				isFast( int ) const;

};

#endif
