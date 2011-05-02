#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.5 2011-05-02 14:25:14 cvsbruno Exp $
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

inline float getLayerDepth( const AIModel& mod, int layer ) 
{
    float depth = 0;
    for ( int idx=0; idx<layer+1; idx++ )
	depth += mod[idx].thickness_;

    return depth;
}


#endif
