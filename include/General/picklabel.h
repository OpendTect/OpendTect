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

/*!\brief Group Label that can be attached to pick locations via the ID.  */

mExpClass(General) GroupLabel
{
public:

    mDefIntegerIDType(		ID);

				GroupLabel( const char* t=0 )
				    : txt_(t)		{}

				mImplSimpleEqOpers1Memb(GroupLabel,id_)

    inline ID			id() const	{ return id_; }
    inline BufferString		text() const	{ return txt_; }

    inline GroupLabel&		setID( ID i )
				{ id_ = i; return *this; }
    inline GroupLabel&		setText( const char* t )
				{ txt_ = t; return *this; }

    void			fillPar(IOPar&,int nr) const;
    bool			usePar(const IOPar&,int nr);
    static void			removeFromPar(IOPar&);

protected:

    ID			id_;
    BufferString	txt_;

			GroupLabel( ID i, const char* t )
			    : id_(i), txt_(t)	{}

    friend class	Set;

};


} // namespace Pick
