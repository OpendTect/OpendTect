#ifndef attribdescid_h
#define attribdescid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2005
 RCS:           $Id$
________________________________________________________________________

-*/

/*! \brief
AttribDesc ID: each Desc has a unique ID in a set; the boolean isstored allows
us to know whether this Desc is to be found in the DescSet dedicated for stored
Attributes.
*/

#include "attributeenginemod.h"
#include "commondefs.h"

namespace Attrib
{

mClass(AttributeEngine) DescID
{
public:
			DescID()
			    : id_(-1)
			    , isstored_(false)			{}
			DescID( int id, bool isstored )
			    : id_(id)
			    , isstored_(isstored)		{}
    			DescID( const DescID& id )
			    : id_(id.id_)
			    , isstored_(id.isstored_)		{}
    inline DescID&	operator =( const DescID& id )
			{ id_ = id.id_; isstored_ = id.isstored_; return *this;}
    inline bool		isValid() const		{ return id_ >= 0; }
    inline bool		isUnselInvalid() const	{ return id_ < -2; }

    /*
    inline bool		operator <(int id) const
			{ return id_ < id; }
    inline bool		operator >(int id) const
			{ return id_ > id; }
    inline bool		operator >=(int id) const
			{ return id_ >= id; }
			*/

    inline bool		operator ==( const DescID& id ) const
			{ return id.id_ == id_ && isstored_ == id.isstored_; }
    inline bool		operator !=( const DescID& id ) const
			{ return id.id_ != id_ || isstored_ != id.isstored_;; }

    static inline DescID undef()		{ return DescID(-1,false); }

    int			asInt() const		{ return id_; }
    int&		asInt()			{ return id_; }
    bool		isStored() const	{ return isstored_; }
    void                setStored( bool yn )    { isstored_ = yn; }

protected:

    int			id_;
    bool		isstored_;
};

} // namespace Attrib

#endif

