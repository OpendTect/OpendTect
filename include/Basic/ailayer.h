#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.7 2011/08/10 15:03:51 cvsbruno Exp $
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


mClass ElasticLayer : public AILayer
{
public:
		ElasticLayer( float thkness, float pvel, float svel, float den )
		    : AILayer(thkness,pvel,den), svel_(svel) {}
		ElasticLayer(const AILayer& ailayer)
		    : AILayer(ailayer), svel_(mUdf(float)) {}

    bool	operator ==( const ElasticLayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	svel_;
};


/*!\brief A table of elastic prop layers */

typedef TypeSet<ElasticLayer> ElasticModel;

#endif
