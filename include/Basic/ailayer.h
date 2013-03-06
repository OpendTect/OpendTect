#ifndef ailayer_h
#define ailayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commondefs.h"
#include "math.h"
#include "sets.h"

/*!
\brief Acoustic Impedance layer.
*/


mExpClass(Basic) AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den)	{}
    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_; }
    
    float	thickness_, vel_, den_;
    float	getAI() const;
    

};


typedef TypeSet<AILayer> AIModel;

mGlobal(Basic) float getLayerDepth( const AIModel& mod, int layer );



/*!
\brief A table of elastic prop layers.
*/

mExpClass(Basic) ElasticLayer : public AILayer
{
public:
		ElasticLayer( float thkness, float pvel, float svel, float den )
		    : AILayer(thkness,pvel,den), svel_(svel) {}
		ElasticLayer(const AILayer& ailayer)
		    : AILayer(ailayer), svel_(mUdf(float)) {}

    bool	operator ==( const ElasticLayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	svel_;
    float	getSI() const;
};


/*!\brief A table of elastic prop layers */
typedef TypeSet<ElasticLayer> ElasticModel;


// ensures a model does not have layers below a given thickness
// last layer may not comply though
mGlobal(Basic) void	upscaleElasticModel(const ElasticModel& inmdl,
				    ElasticModel& oumdl,float maxthickness);

// Smashes every consecutive set of nblock layers into one output layer
mGlobal(Basic) void	upscaleElasticModelByN(const ElasticModel& inmdl,
				       ElasticModel& oumdl,int nblock);

#endif

