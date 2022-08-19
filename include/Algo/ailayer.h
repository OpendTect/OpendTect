#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "math.h"
#include "ranges.h"
#include "survinfo.h"
#include "typeset.h"

template <class T> class Array2DImpl;

mGlobal(Algo) inline float cMinLayerThickness() { return 1e-4f; }
mGlobal(Algo) inline const Interval<float> validThicknessRange();
mGlobal(Algo) inline const Interval<float> validDensityRange();
mGlobal(Algo) inline const Interval<float> validVelocityRange();
mGlobal(Algo) inline const Interval<float> validImpRange();


/*!
\brief Acoustic Impedance layer.
*/


mExpClass(Algo) AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den) {}

		//Velocity will be computed using Gardner equation
		//in case density is undef.
		AILayer(float thkness,float ai, float den,
			bool needcompthkness);

    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	thickness_;
    float	vel_;
    float	den_;

    float	getAI() const;
    bool	isOK(bool dodencheck=true) const;
    bool	isValidVel() const;
    bool	isValidDen() const;

		//Compute Den from Gardner
    bool	fillDenWithVp(bool onlyinvalid);

};


typedef TypeSet<AILayer> AIModel;

mGlobal(Algo) float getLayerDepth(const AIModel& mod,int layer);



/*!
\brief A table of elastic prop layers.
*/

mExpClass(Algo) ElasticLayer : public AILayer
{
public:
		ElasticLayer(float thkness,float pvel,float svel,float den);

		//To be used only for 0 offsets
		ElasticLayer(const AILayer&);
		ElasticLayer(float thkness,float ai,float si,
			     float den,bool needcompthkness);

    bool	operator ==( const ElasticLayer& p ) const
		{ return thickness_ == p.thickness_; }

    float	svel_;
    float	getSI() const;

    bool	isOK(bool dodencheck=true,bool dosvelcheck=true) const;
    bool	isValidVs() const;

		//Compute Vs from Castagna
    bool	fillVsWithVp(bool onlyinvalid);
};


/*!\brief A table of elastic prop layers with processing utilities*/
mExpClass(Algo) ElasticModel : public TypeSet<ElasticLayer>
{
public:

		/*! Checks if all layers have valid property values
		    returns index of first invalid layer */

    int		isOK(bool dodencheck=true,bool dosvelcheck=true) const;

		/*! Get one of the properties */

    bool	getValues(bool isden,bool issvel,TypeSet<float>&) const;

		/*! Get several properties, in the order vel - den - svel */

    bool	getValues(bool vel,bool den,bool svel,
			  Array2DImpl<float>&) const;

    void	checkAndClean(int& firsterroridx,bool dodencheck=true,
			      bool dosvelcheck=true,bool onlyinvalid=false);

		/*! Replaces all undefined or invalid values */

    void	interpolate(bool dovp,bool doden,bool dovs);

		/*! Ensures a model does not have layers below a given thickness
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
		  \param relthreshold
		  \param pvelonly Will use density and SVel as well if false */

    void	block(float relthreshold,bool pvelonly);

		/*! compute an upscaled elastic layer from an elastic model
		  using simple weighted averaging.
		  The thickness of the input and output remains constant.
		  returns false if the input model does not contain a single
		  valid input layer */

    bool	getUpscaledByThicknessAvg(ElasticLayer& outlay) const;

		/* computes an upscaled elastic layer from an elastic model
		   using backus upscaling method. The thickness of the input and
		   output remains constant.
		   returns false if the input model does not contain a single
		   valid input layer
		   \param theta Incidence angle in radians */

    bool	getUpscaledBackus(ElasticLayer& outlay,float theta=0.) const;

    bool	createFromVel(const StepInterval<float>& zrange,
			      const float* pvel, const float* svel=0,
			      const float* den=0);

    bool	createFromAI(const StepInterval<float>& zrange,const float* ai,
			     const float* si =0,const float* den =0);

		/* Return depth of the middle of the layer */

    float	getLayerDepth(int layerix) const;


    static bool getTimeSampling(const TypeSet<ElasticModel>&,
				Interval<float>& timerg,bool usevs=false);

    void	getTimeSampling(Interval<float>&,bool usevs=false) const;


protected:

		/*! Computes first derivative of the elastic properties
		    May also return the input values */
    bool	getRatioValues(bool vel,bool den,bool svel,
			       Array2DImpl<float>& ratiovals,
			       Array2DImpl<float>* vals=0) const;

		/* Gives layer index distributions of similar properties */

    bool	doBlocking(float threshold,bool pvelonly,
			   TypeSet<Interval<int> >& blocks) const;

		/* If a layer thickness is strictly identical to zstep,
		   maybe split that layer over the last/previous layers */

    void	removeSpuriousLayers(float zstep);


};

inline const Interval<float> validThicknessRange()
{ return Interval<float> ( cMinLayerThickness(), mUdf(float) ); }

inline const Interval<float> validDensityRange()
{ return Interval<float> ( 100.f, 10000.f ); }

inline const Interval<float> validVelocityRange()
{ return Interval<float> ( 10.f, 10000.f ); }

inline const Interval<float> validImpRange()
{
    return Interval<float> (
	    validDensityRange().start*validVelocityRange().start,
	    validDensityRange().stop*validVelocityRange().stop );
}
