#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: ailayer.h,v 1.10 2012-06-12 10:02:31 cvsbruno Exp $
________________________________________________________________________

-*/

/*!\brief Acoustic Impedance layer.  */

#include "commondefs.h"
#include "math.h"
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


static void blockElasticModel( ElasticModel& mdl, float threshold )
{
    for ( int idx=mdl.size()-1; idx>=1; idx-- )
    {
	const float veldiff = mdl[idx].vel_ - mdl[idx-1].vel_;
	if ( fabs( veldiff ) < threshold )
	{
	    const float thk = mdl[idx].thickness_;
	    if ( idx == mdl.size() -1 )
		mdl[idx-1].thickness_ += thk;
	    else
		mdl[idx+1].thickness_ += thk;

	    mdl.remove( idx -1 );
	}
    }
}


#endif
