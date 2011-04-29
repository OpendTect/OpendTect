#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.4 2011-04-29 14:12:05 cvsbruno Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

#include "commondefs.h"
#include "sets.h"

mClass AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	thickness_, vel_, den_;
};


/*!\brief A table of elastic prop layers */

typedef TypeSet<AILayer> AIModel;


#endif
