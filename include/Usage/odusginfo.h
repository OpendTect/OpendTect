#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusginfo.h,v 1.3 2009-06-30 15:23:47 cvsbert Exp $
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

    typedef int		ID;

    			Info( const char* grp, const char* act=0,
			      const char* aux=0 )
			    : id_(newID())
			    , group_(grp)
			    , action_(act)
    			    , aux_(aux)
    			    , start_(true)
    			    , wantreply_(false)		{}

    bool		operator ==( const Info& inf ) const
			{ return id_ == inf.id_; }

    ID			id_;		//!< Unique ID
    BufferString	group_;		//!< Action group
    BufferString	action_;	//!< Specific action
    BufferString	aux_;		//!< Extra info
    bool		start_;		//!< Is this start of action?
    bool		wantreply_;	//!< Wait for a reply?

    std::ostream&	dump(std::ostream&) const;
    BufferString&	dump(BufferString&) const;

protected:

    static ID		newID();

};


} // namespace


#endif
