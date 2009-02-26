#ifndef attribdescid_h
#define attribdescid_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          July 2005
 RCS:           $Id: attribdescid.h,v 1.4 2009-02-26 13:00:52 cvsbert Exp $
________________________________________________________________________

-*/

/*! \brief
AttribDesc ID
*/

#include "commondefs.h"

namespace Attrib
{

mClass DescID
{
public:
			DescID() : id_(-1)			{}
			DescID( int id, bool ) : id_(id)	{}
			//!< The bool is there to prevent accidental
			//!< conversion from int
    			DescID( const DescID& id )
			    : id_(id.id_)	{}
    inline DescID&	operator =( const DescID& id )
			{ id_ = id.id_; return *this; }
    inline bool		isValid() const		{ return id_ >= 0; }
    inline bool		isUnselInvalid() const	{ return id_ < -1; }

    /*
    inline bool		operator <(int id) const
			{ return id_ < id; }
    inline bool		operator >(int id) const
			{ return id_ > id; }
    inline bool		operator >=(int id) const
			{ return id_ >= id; }
			*/

    inline bool		operator ==( const DescID& id ) const
			{ return id.id_ == id_; }
    inline bool		operator !=( const DescID& id ) const
			{ return id.id_ != id_; }

    static inline DescID undef()		{ return DescID(-1,true); }

    int			asInt() const		{ return id_; }
    int&		asInt()			{ return id_; }

protected:

    int			id_;
};

} // namespace Attrib

#endif
