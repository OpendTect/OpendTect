#ifndef attribslice_h
#define attribslice_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: attribslice.h,v 1.1 2005-02-04 09:28:35 kristofer Exp $
________________________________________________________________________

-*/

#include <sets.h>
#include <arrayndimpl.h>
#include <cubesampling.h>
#include <refcount.h>

/*!\brief Slice containing attribute values.
 
  The sliceidx determines the position of the slice in the requested cube,
  see AttribSliceSet for details.
 
 */

namespace Attrib
{

class Slice : public Array2DImpl<float>
{ mRefCountImplNoDestructor(Slice);
public:

    enum Dir		{ Hor, Inl, Crl };

    			Slice( int nrows, int ncols, float udfval=0 );
    float		undefValue() const;
    void		setUndefValue( float udfval, bool initdata=false );

protected:

    float		udfval_;

};


/*!\brief Set of attrib slices.
 
  The two array2d directions shall be filled as follows:

	Dir |	Dim1	Dim2
	----|---------------
	Hor |	Inl	Crl
	Inl |	Crl	Z
	Crl |	Inl	Z

  Slices can be null!

  The slices will be in order of increasing inl, crl or Z.
 
 */

class SliceSet : public ObjectSet<Slice>
{ mRefCountImpl(SliceSet);
public:

			SliceSet();

    Slice::Dir		direction;
    CubeSampling	sampling;

    int			dim(int,Slice::Dir) const;
    static		Slice::Dir defaultDirection(const CubeSampling&);

    int			dimNr(Slice::Dir) const;
    void		getIdx(int dimnr,int inl,int crl,float z,int&) const;
    void		getIdxs(int inl,int crl,float z,int&,int&,int&) const;

    Array3D<float>*	createArray(int inldim=0, int crlcim=1,
	    			    int depthdim=2) const;
    			/*!< Makes an array where the dims are as specified
			 */

};

}; //namespace

#endif
