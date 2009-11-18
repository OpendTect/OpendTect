#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusginfo.h,v 1.6 2009-11-18 15:00:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include <iosfwd>


namespace Usage
{

/*!\brief Info going to and coming from server. */

mClass Info
{
public:

    typedef od_uint64	ID;

    			Info( const char* grp, const char* act=0,
			      const char* aux=0 )
			    : id_(-1)
			    , group_(grp)
			    , action_(act)
    			    , aux_(aux)
    			    , start_(true)
    			    , withreply_(false)		{}

    bool		operator ==( const Info& inf ) const
			{ return id_ == inf.id_; }

    BufferString	group_;		//!< Action group
    BufferString	action_;	//!< Specific action
    BufferString	aux_;		//!< Extra info
    bool		start_;		//!< Initiate action or close it?
    bool		withreply_;	//!< Client side: wait for a reply?

    ID			id() const	{ return id_; }

    std::ostream&	dump(std::ostream&) const;
    BufferString&	dump(BufferString&) const;

    void		prepStart( const char* act=0 )
    			{ aux_.setEmpty(); if ( act ) action_ = act;
			  start_ = true; }
    void		prepEnd( const char* act=0 )
    			{ aux_.setEmpty(); if ( act ) action_ = act;
			  start_ = withreply_ = false; }

protected:

    static ID		newID();
    ID			id_;		//!< Unique ID

    friend class	Client;

};


} // namespace


#endif
