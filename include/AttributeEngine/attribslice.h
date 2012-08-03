#ifndef attribslice_h
#define attribslice_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: attribslice.h,v 1.9 2012-08-03 13:00:08 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "sets.h"
#include "arrayndimpl.h"
#include "cubesampling.h"
#include "refcount.h"

/*!\brief Slice containing attribute values.
 
  The sliceidx determines the position of the slice in the requested cube,
  see AttribSliceSet for details.
 
 */

namespace Attrib
{

mClass(AttributeEngine) Slice : public Array2DImpl<float>
{ mRefCountImplNoDestructor(Slice);
public:

    			Slice(int nrows,int ncols,float udfval=0);
    float		undefValue() const;
    void		setUndefValue( float udfval, bool initdata=false );

protected:

    float		udfval_;

};


/*!\brief Set of attrib slices.

 The two array2d directions shall be filled following the CubeSampling
 convention. The slices will be in order of increasing inl, crl or Z.
 
 Slices can be null!
 
 */

mClass(AttributeEngine) SliceSet : public ObjectSet<Slice>
{ mRefCountImpl(SliceSet);
public:

			SliceSet();

    CubeSampling::Dir	direction_;
    CubeSampling	sampling_;

    void		getIdx(int dimnr,int inl,int crl,float z,int&) const;
    void		getIdxs(int inl,int crl,float z,int&,int&,int&) const;

    Array3D<float>*	createArray(int inldim=0, int crlcim=1,
	    			    int depthdim=2) const;
    			/*!< Makes an array where the dims are as specified
			 */
    float               getValue(int inl,int crl,float z) const;

};

}; //namespace

#endif

