#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2005
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "integerid.h"

namespace Attrib
{

/*!\brief unique ID in a DescSet combined with whether it's stored.  */

class DescID : public ::IntegerID<int>
{
public:

    inline		DescID()		{}
    inline explicit	DescID( int n )
			    : IntegerID<int>(n)	{}
    inline explicit	operator int() const	{ return this->nr_; }

    inline bool		operator ==( const DescID& oth ) const
			{ return oth.nr_ == nr_; }
    inline bool		operator !=( const DescID& oth ) const
			{ return oth.nr_ != nr_; }

    inline bool		isUnselInvalid() const	{ return nr_ < -2; }
    inline void		setUnselInvalid()	{ nr_ = -3; }

    static inline DescID getInvalid()		{ return DescID(); }

    mDeprecated static DescID	undef()		{ return getInvalid(); }
    mDeprecated int&	asInt()			{ return getI(); }
    mDeprecated int	asInt() const		{ return getI(); }

};

} // namespace Attrib
