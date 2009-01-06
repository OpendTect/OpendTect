#ifndef stratlith_h
#define stratlith_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratlith.h,v 1.6 2009-01-06 05:34:47 cvsranojay Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "repos.h"

namespace Strat
{

/*!\brief name and integer ID (in well logs). */

mClass Lithology : public ::NamedObject
{
public:

			Lithology( const char* nm=0 )
			: NamedObject(nm)
			, id_(0)
			, porous_(false)
			, src_(Repos::Temp)		{}

     int		id() const			{ return id_; }
     void		setId( int i )			{ id_ = i; }
     bool		isPorous() const		{ return porous_; }
     void		setIsPorous( bool yn )		{ porous_ = yn; }
     Repos::Source	source() const			{ return src_; }
     void		setSource( Repos::Source s )	{ src_ = s; }

     static const Lithology& undef();

     void		fill(BufferString&) const;
     bool		use(const char*);

protected:

    int			id_;
    bool		porous_;
    Repos::Source	src_;

};


}; // namespace Strat

#endif
