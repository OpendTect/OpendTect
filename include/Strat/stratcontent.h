#ifndef stratcontent_h
#define stratcontent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
 RCS:		$Id: stratcontent.h,v 1.2 2012-01-24 16:40:14 cvsbert Exp $
________________________________________________________________________

 Impl is in stratlith.cc.

-*/

#include "objectset.h"


namespace Strat
{

/*!\brief stuff that can be inside porous layers */

mClass Content
{
public:

    typedef int		ID;

			Content()
			    : id_(unspecified().id_)
			    , name_(unspecified().name_)	{}
			Content( const char* nm )
			    : id_(nextID()), name_(nm)		{}
			Content( ID id, const char* nm )
			    : id_(id), name_(nm)		{}
			Content( const Content& c )
			    : id_(c.id_), name_(c.name_)	{}
    Content&		operator =( const Content& c )
			{ const_cast<ID&>(id_) = c.id_; name_ = c.name_;
			    return *this; }
    bool		operator ==( const Content& c ) const
    			{ return id_ == c.id_; }

    const ID		id_;
    BufferString	name_;
    inline bool		isUnspecified() const
    			{ return id_ == unspecified().id_; }

    static const Content& unspecified();
    static ID		nextID();
    static void		setLastID(ID);

};


/*!\brief set of that can be inside porous layers */

mClass ContentSet : public ObjectSet<Content>
{
public:

    			~ContentSet()		{ deepErase(*this); }

    int		getIndexOf(Content::ID) const;
    int		getIndexOf(const char*) const;

    template <class T> Content*		get( const T& t )
    { const int idx = getIndexOf(t); return idx<0 ? 0 : (*this)[idx]; }
    template <class T> const Content*	get( const T& t ) const
    { const int idx = getIndexOf(t); return idx<0 ? 0 : (*this)[idx]; }

};


} // namespace Strat

#endif
