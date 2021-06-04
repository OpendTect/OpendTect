#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
________________________________________________________________________

 Impl is in stratlith.cc.

-*/

#include "stratmod.h"
#include "namedobj.h"
#include "objectset.h"
#include "draw.h"

/*!\brief Stratigraphy*/

namespace Strat
{

/*!\brief stuff that can be inside porous layers */

mExpClass(Strat) Content : public NamedObject
{
public:

			Content( const char* nm )
			    : NamedObject(nm)				{}
			Content( const Content& c )
			    : NamedObject(c)
			    , pattern_(c.pattern_)
			    , color_(c.color_)				{}

    Content&		operator =( const Content& c )
			{
			    setName(c.name());
			    pattern_=c.pattern_;
			    color_ = c.color_;
			    return *this;
			}
    bool		operator ==( const Content& c ) const
			{ return name() == c.name(); }

    inline bool		isUnspecified() const
			{ return this == &unspecified(); }

    static const Content& unspecified();

    OD::Color		color_;
    FillPattern		pattern_;

    bool		getApearanceFrom(const char*);
    void		putAppearanceTo(BufferString&) const;

};


//!\brief convenience: contents can be null or unspecified, handle in one go
mGlobal(Strat) inline bool isUnspecified( const Content* ct )
{
    return !ct || ct->isUnspecified();
}


/*!\brief set of names for stuff that can be inside porous layers */

mExpClass(Strat) ContentSet : public ObjectSet<Content>
{
public:

			~ContentSet()		{ deepErase(*this); }

    int			getIndexOf(const char*) const;

    Content*		getByName( const char* s )
    { const int idx = getIndexOf(s); return idx<0 ? 0 : (*this)[idx]; }
    const Content*	getByName( const char* s ) const
    { return const_cast<ContentSet*>(this)->getByName(s); }

    const Content&	getContent( const char* s ) const
    { const Content* c = getByName(s); return c ? *c : Content::unspecified(); }

};


} // namespace Strat

