#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.15 2012-08-28 12:13:45 cvskris Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

#include "basicmod.h"
#include "commondefs.h"
#include "math.h"
#include "sets.h"


class AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	thickness_, vel_, den_;

    float	getAI() const 		{ return vel_*den_; }
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


class ElasticLayer : public AILayer
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


inline void blockElasticModel( ElasticModel& mdl, float threshold )
{
    float velthreshold = threshold;
    float denthreshold = threshold;
    for ( int idx=mdl.size()-1; idx>=1; idx-- )
    {
	const float veldiff = mdl[idx].vel_ - mdl[idx-1].vel_;
	const float dendiff = mdl[idx].den_ - mdl[idx-1].den_;
	if ( fabs( veldiff ) < velthreshold && fabs( dendiff ) < denthreshold )
	{
	    mdl[idx-1].thickness_ += mdl[idx].thickness_;
	    mdl.remove( idx );
	}
    }
}


#endif

