#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "sets.h"
#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "refcount.h"

namespace Attrib
{

/*!
\brief Slice containing attribute values.
  
  The sliceidx determines the position of the slice in the requested cube,
  see AttribSliceSet for details.    
*/


mExpClass(AttributeEngine) Slice : public Array2DImpl<float>
{ mRefCountImplNoDestructor(Slice);
public:

    			Slice(int nrows,int ncols,float udfval=0);
    float		undefValue() const;
    void		setUndefValue( float udfval, bool initdata=false );

protected:

    float		udfval_;

};


/*!
\brief Set of attrib slices.
  
  The two array2d directions shall be filled following the TrcKeyZSampling
  convention. The slices will be in order of increasing inl, crl or Z.
  
  Slices can be null!
*/

mExpClass(AttributeEngine) SliceSet : public ObjectSet<Slice>
{ mRefCountImpl(SliceSet);
public:

			SliceSet();

    TrcKeyZSampling::Dir	direction_;
    TrcKeyZSampling	sampling_;

    void		getIdx(int dimnr,int inl,int crl,float z,int&) const;
    void		getIdxs(int inl,int crl,float z,int&,int&,int&) const;

    Array3D<float>*	createArray(int inldim=0, int crlcim=1,
	    			    int depthdim=2) const;
    			/*!< Makes an array where the dims are as specified
			 */
    float               getValue(int inl,int crl,float z) const;

};

}; //namespace

