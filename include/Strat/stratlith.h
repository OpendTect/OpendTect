#ifndef stratlith_h
#define stratlith_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratlith.h,v 1.2 2004-11-29 17:04:26 bert Exp $
________________________________________________________________________


-*/

#include "uidobj.h"

namespace Strat
{

/*!\brief name and integer ID (in well logs). */

class Lithology : public ::UserIDObject
{
public:

			Lithology( const char* nm=0 )
			: UserIDObject(nm)
			, id_(0)		{}

     int		Id() const		{ return id_; }
     void		setId( int i )		{ id_ = i; }

     static const Lithology& undef();

protected:

    int			id_;

};


}; // namespace Strat

#endif
