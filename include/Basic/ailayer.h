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


/*!\brief A table of elastic prop layers with processing utilities*/
mExpClass(Basic) ElasticModel : public TypeSet<ElasticLayer>
{
public:

		/*! ensures a model does not have layers below a given thickness
		  last layer may not comply though */

    void	upscale(float maxthickness, ElasticModel& outmdl);
    void	upscale( float maxthickness )
			    { return upscale( maxthickness, *this ); }

		/*! Smashes every consecutive set of nblock layers
		  into one output layer */

    void	upscaleByN(int nblock, ElasticModel& outmdl);
    void	upscaleByN( int nblock )
			    { return upscaleByN( nblock, *this ); }


		/*! Block elastic model so that no blocks have larger difference
		  than the threshold. Attempts will be made to put boundaries at
		  large changes.
		  \param pvelonly Will use density and SVel as well if false */

    void	block(float relthreshold,bool pvelonly,ElasticModel& outmdl);
    void	block( float relthreshold, bool pvelonly )
			    { return block( relthreshold, pvelonly, *this ); }


		/*! compute an upscaled elastic layer from an elastic model
		  using simple weighted averaging.
		  The thickness of the input and output remains constant.
		  returns false if the input model contain not a single valid
		  input layer */

    bool	upscaleByThicknessAvg(ElasticModel& inmdl,ElasticLayer& outlay);
    bool	upscaleByThicknessAvg( ElasticLayer& outlay )
			    { return upscaleByThicknessAvg( *this, outlay ); }


		/* compute and upscaled elastic layer from an elastic modelusing
		   backus upscaling method. The thickness of the input and
		   output remains constant.
		   returns false if the input model contain not a single valid
		   input layer
		   \param theta Incidence angle in radians */

    bool	upscaleBackus(ElasticModel& inmdl, ElasticLayer& outlay,
	    		      float theta=0.);
    bool	upscaleBackus( ElasticLayer& outlay, float theta=0. )
			    { return upscaleBackus( *this, outlay, theta ); }


		/*!Ensures that all layers in the elastic model are not thicker
		  than a maximum thickness. Splits the blocks if necessary */

    void	setMaxThickness(float maxthickness, ElasticModel& outmdl);
    void	setMaxThickness( float maxthickness )
			    { return setMaxThickness( maxthickness, *this ); }


};

#endif

