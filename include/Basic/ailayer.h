#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.2 2011-01-31 06:21:44 cvsranojay Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

#include "commondefs.h"

mClass AILayer
{
public:
		AILayer( float z, float vel, float den )
		    : depth_(z), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return depth_ == p.depth_; }

    float	depth_, vel_, den_;
};


#endif
