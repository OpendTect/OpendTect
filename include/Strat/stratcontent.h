#ifndef stratcontent_h
#define stratcontent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
 RCS:		$Id: stratcontent.h,v 1.5 2012-09-13 11:27:53 cvsbert Exp $
________________________________________________________________________

 Impl is in stratlith.cc.

-*/

#include "stratmod.h"
#include "namedobj.h"
#include "objectset.h"
#include "color.h"


namespace Strat
{

/*!\brief stuff that can be inside porous layers */

mClass(Strat) Content : public NamedObject
{
public:

			Content( const char* nm )
			    : NamedObject(nm)
			    , pattype_(0), patopt_(0)	{}
			Content( const Content& c )
			    : NamedObject(c)
			    , pattype_(c.pattype_)
			    , patopt_(c.patopt_)	{}
    Content&		operator =( const Content& c )
			{ setName( c.name() ); pattype_ = c.pattype_;
			    patopt_ = c.patopt_; return *this; }
    bool		operator ==( const Content& c ) const
    			{ return name() == c.name(); }

    inline bool		isUnspecified() const
    			{ return this == &unspecified(); }

    static const Content& unspecified();

    int			pattype_;
    int			patopt_;
    Color		color_;

    bool		getApearanceFrom(const char*);
    void		putAppearanceTo(BufferString&) const;

};


/*!\brief set of names for stuff that can be inside porous layers */

mClass(Strat) ContentSet : public ObjectSet<Content>
{
public:

    			~ContentSet()		{ deepErase(*this); }

    int			getIndexOf(const char*) const;

    Content*		getByName( const char* s )
    { const int idx = getIndexOf(s); return idx<0 ? 0 : (*this)[idx]; }
    const Content*	getByName( const char* s ) const
    { return const_cast<ContentSet*>(this)->getByName(s); }

    const Content&	get( const char* s ) const
    { const Content* c = getByName(s); return c ? *c : Content::unspecified(); }

};


} // namespace Strat

#endif

