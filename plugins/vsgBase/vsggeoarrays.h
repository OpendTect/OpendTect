#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <vsg/core/Array.h>

#include <algorithm>
#include <cstddef>

namespace vsgGeo
{

/*!\brief Vertex, colour or normal array that changes at runtime.

  A compiled vsg array has a GPU buffer of fixed size. This keeps spare
  capacity, so that changing the number of elements only needs a new
  array (and a recompile) when the capacity is exceeded. The owner draws
  size() elements, not the full array.

  After changing values, call dirty() so VSG uploads them.
*/

template <class ArrayT>
class DynamicArray
{
public:

    using value_type = typename ArrayT::value_type;

			DynamicArray(size_t minsize=16)
			    : minsize_(std::max<size_t>(minsize,1))
			{}

    bool		resize(size_t newsize)
			/*!<\returns true if a new array was allocated; the
			    owner must then use array() again and have the
			    subgraph recompiled. */
			{
			    if ( newsize <= capacity() )
			    {
				size_ = newsize;
				return false;
			    }

			    size_t newcap = std::max( minsize_, capacity()*2 );
			    while ( newcap < newsize )
				newcap *= 2;

			    vsg::ref_ptr<ArrayT> newarr =
					    ArrayT::create( uint32_t(newcap) );
			    newarr->properties.dataVariance =
						    vsg::DYNAMIC_DATA;
			    for ( size_t idx=0; idx<size_; idx++ )
				newarr->at( idx ) = arr_->at( idx );

			    arr_ = newarr;
			    size_ = newsize;
			    return true;
			}

    size_t		size() const		{ return size_; }
    size_t		capacity() const
			{ return arr_ ? arr_->size() : 0; }

    value_type&		operator[]( size_t idx )       { return arr_->at(idx); }
    const value_type&	operator[]( size_t idx ) const { return arr_->at(idx); }

    ArrayT*		array()			{ return arr_.get(); }
    vsg::ref_ptr<ArrayT> arrayPtr()		{ return arr_; }

    void		dirty()			{ if ( arr_ ) arr_->dirty(); }

private:

    vsg::ref_ptr<ArrayT> arr_;
    size_t		size_ = 0;
    size_t		minsize_;
};

} // namespace vsgGeo
