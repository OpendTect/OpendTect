#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
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

    enum Delim		{ Start, Stop, Cont };

    mClass ID
    {
    public:
			ID();					//!< client
			ID(od_uint64,const char* hnm,int);	//!< server

	ID&		operator =(const ID&);
	bool		operator ==(const ID&) const;

	od_uint64	nr_;
	BufferString	hostname_;
	int		pid_;

	bool		isLocal() const;
	void		putTo(BufferString&) const;
	bool		getFrom(const char*);

    };

    			Info(const char* grp,const char* act=0,
			     const char* aux=0);

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

    ID			id_;

    friend class	Client;
    void		prepareForSend();

};


} // namespace


#endif
