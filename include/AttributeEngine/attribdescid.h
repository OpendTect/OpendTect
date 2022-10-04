#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "gendefs.h"

namespace Attrib
{

/*!
\brief AttribDesc ID: each Desc has a unique ID in a set; the boolean
isstored allows us to know whether this Desc is to be found in the DescSet
dedicated for stored Attributes.
*/

mExpClass(AttributeEngine) DescID
{
public:
			DescID();
			DescID(int id,bool isstored);
			DescID(const DescID&);
			~DescID();

    inline DescID&	operator =( const DescID& id )
			{ id_ = id.id_; isstored_ = id.isstored_; return *this;}
    inline bool		isValid() const		{ return id_ >= 0; }
    inline bool		isUnselInvalid() const	{ return id_ < -2; }

    inline bool		operator ==( const DescID& id ) const
			{ return id.id_ == id_ && isstored_ == id.isstored_; }
    inline bool		operator !=( const DescID& id ) const
			{ return id.id_ != id_ || isstored_ != id.isstored_;; }

    static inline DescID undef()		{ return DescID(-1,false); }

    int			asInt() const		{ return id_; }
    int&		asInt()			{ return id_; }
    bool		isStored() const	{ return isstored_; }
    void		setStored( bool yn )	{ isstored_ = yn; }

protected:

    int			id_;
    bool		isstored_;
};

} // namespace Attrib
