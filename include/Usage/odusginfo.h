#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusginfo.h,v 1.7 2009-11-19 12:17:59 cvsbert Exp $
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
    enum Delim		{ Start, Stop, Cont };

    			Info( const char* grp, const char* act=0,
			      const char* aux=0 )
			    : id_(-1)
			    , group_(grp)
			    , action_(act)
    			    , aux_(aux)
    			    , delim_(Start)
    			    , withreply_(false)		{}

    bool		operator ==( const Info& inf ) const
			{ return id_ == inf.id_; }

    BufferString	group_;		//!< Action group
    BufferString	action_;	//!< Specific action
    BufferString	aux_;		//!< Extra info
    Delim		delim_;
    bool		withreply_;	//!< Client waits for a reply?

    ID			id() const	{ return id_; }

    std::ostream&	dump(std::ostream&) const;
    BufferString&	dump(BufferString&) const;

    void		prepStart( const char* act=0 )
    			{ aux_.setEmpty(); if ( act ) action_ = act;
			  delim_ = Start; }
    void		prepEnd( const char* act=0 )
    			{ aux_.setEmpty(); if ( act ) action_ = act;
			  delim_ = Stop; withreply_ = false; }

protected:

    static ID		newID();
    ID			id_;		//!< Unique ID

    friend class	Client;

};


} // namespace


#endif
