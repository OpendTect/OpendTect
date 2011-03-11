#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.3 2011-03-11 13:42:09 cvsbruno Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

#include "commondefs.h"
#include "sets.h"

mClass AILayer
{
public:
		AILayer( float z, float vel, float den )
		    : depth_(z), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return depth_ == p.depth_; }

    float	depth_, vel_, den_;
};


/*!\brief A table of elastic prop layers */

typedef TypeSet<AILayer> AIModel;


#endif
