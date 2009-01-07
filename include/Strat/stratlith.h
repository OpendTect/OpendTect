#ifndef stratlith_h
#define stratlith_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratlith.h,v 1.7 2009-01-07 15:11:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "repos.h"

namespace Strat
{

/*!\brief name and integer ID. */

mClass Lithology : public ::NamedObject
{
public:

			Lithology( const char* nm=0 )
			: NamedObject(nm)
			, id_(-1)
			, porous_(false)
			, src_(Repos::Temp)	{}
			Lithology( const Lithology& l )
						{ *this = l; }
     Lithology&		operator =(const Lithology&);

     static const Lithology& undef();
     bool		isUdf() const		{ return this == &undef(); }

     void		fill(BufferString&) const;
     bool		use(const char*);

     int		id_;
     bool		porous_;
     Repos::Source	src_;

};


}; // namespace Strat

#endif
