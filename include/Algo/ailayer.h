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

#include "algomod.h"
#include "gendefs.h"
#include "math.h"
#include "survinfo.h"
#include "typeset.h"

/*!
\brief Acoustic Impedance layer.
*/


mExpClass(Algo) AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den) {};

		//Velocity will be computed using Gardner equation
		//in case density is undef.
		AILayer(float thkness,float ai, float den,
			bool needcompthkness );

    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	thickness_, vel_, den_;
    float	getAI() const;


};


typedef TypeSet<AILayer> AIModel;

mGlobal(Algo) float getLayerDepth( const AIModel& mod, int layer );



/*!
\brief A table of elastic prop layers.
*/

mExpClass(Algo) ElasticLayer : public AILayer
{
public:
		ElasticLayer( float thkness, float pvel, float svel, float den )
		    : AILayer(thkness,pvel,den), svel_(svel) {}

		//To be used only for 0 offsets
		ElasticLayer(const AILayer& ailayer)
		    : AILayer(ailayer), svel_(mUdf(float)) {}
		ElasticLayer(float thkness,float ai,float si,
			     float den,bool needcompthkness);

    bool	operator ==( const ElasticLayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	svel_;
    float	getSI() const;
};


/*!\brief A table of elastic prop layers with processing utilities*/
mExpClass(Algo) ElasticModel : public TypeSet<ElasticLayer>
{
public:

		/*! ensures a model does not have layers below a given thickness
		  last layer may not comply though */

    void	upscale(float maxthickness);

		/*! Smashes every consecutive set of nblock layers
		  into one output layer */

    void	upscaleByN(int nblock);

		/*! Ensures that all layers in the elastic model are not thicker
		    than a maximum thickness. Splits the blocks if necessary */

    void	setMaxThickness(float maxthickness);

		/*! Merged consecutive layers with same properties. */

    void	mergeSameLayers();

		/*! Block elastic model so that no blocks have larger difference
		  than the threshold. Attempts will be made to put boundaries at
		  large changes.
		  \param pvelonly Will use density and SVel as well if false */

    void	block(float relthreshold,bool pvelonly);

		/*! compute an upscaled elastic layer from an elastic model
		  using simple weighted averaging.
		  The thickness of the input and output remains constant.
		  returns false if the input model contain not a single valid
		  input layer */

    bool	getUpscaledByThicknessAvg(ElasticLayer& outlay) const;

		/* computes an upscaled elastic layer from an elastic model
		   using backus upscaling method. The thickness of the input and
		   output remains constant.
		   returns false if the input model contain not a single valid
		   input layer
		   \param theta Incidence angle in radians */

    bool	getUpscaledBackus(ElasticLayer& outlay,float theta=0.) const;

    bool	createFromVel(const StepInterval<float>& zrange,
			      const float* pvel, const float* svel =0,
			      const float* den =0);

    bool	createFromAI(const StepInterval<float>& zrange,const float* ai,
			     const float* si =0,const float* den =0);


};

#endif

