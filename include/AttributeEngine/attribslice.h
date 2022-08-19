#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				 , public ReferencedObject
{
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
				    , public ReferencedObject
{
public:
				SliceSet();

    TrcKeyZSampling::Dir	direction_;
    TrcKeyZSampling		sampling_;

    void		getIdx(int dimnr,int inl,int crl,float z,int&) const;
    void		getIdxs(int inl,int crl,float z,int&,int&,int&) const;

    Array3D<float>*	createArray(int inldim=0, int crlcim=1,
	    			    int depthdim=2) const;
    			/*!< Makes an array where the dims are as specified
			 */
    float		getValue(int inl,int crl,float z) const;

protected:
			~SliceSet();
};

} // namespace Attrib
