#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"
#include "integerid.h"


namespace Pick
{

/*!\brief Label that can be attached to pick locations via the ID.  */

mExpClass(General) Label
{
public:

    mDefIntegerIDType(int,	ID);

				Label( const char* t=0 )
				    : txt_(t)		{}

    bool			operator ==( const Label& oth ) const
				{ return id_ == oth.id_; }
    inline bool			operator !=( const Label& oth ) const
				{ return !(*this == oth); }

    inline ID			id() const	{ return id_; }
    inline BufferString		text() const	{ return txt_; }

    inline Label&		setID( ID i )
				{ id_ = i; return *this; }
    inline Label&		setText( const char* t )
				{ txt_ = t; return *this; }

protected:

    ID			id_;
    BufferString	txt_;

			Label( ID i, const char* t )
			    : id_(i), txt_(t)	{}

    friend class	Set;

};


} // namespace Pick
