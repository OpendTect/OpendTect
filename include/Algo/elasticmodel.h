#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Someone / Bert
 Date:		Nov 2010 / Sep 2018
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "math.h"
#include "ranges.h"
#include "survinfo.h"
#include "typeset.h"
#include "manobjectset.h"

template <class T> class Array2DImpl;

inline float cMinLayerThickness()
	{ return 1e-4f; }

inline Interval<float> validThicknessRange()
	{ return Interval<float>( cMinLayerThickness(), 1.e6f ); }

inline Interval<float> validDensityRange()
	{ return Interval<float>( 100.f, 10000.f ); }

inline Interval<float> validVelocityRange()
	{ return Interval<float> ( 10.f, 10000.f ); }

inline const Interval<float> validImpedanceRange()
	{ return Interval<float>( 1000.f, 1.e8f ); }

#define mIsValidThickness(val)	validThicknessRange().includes( val, false )
#define mIsValidDensity(val)	validDensityRange().includes( val, false )
#define mIsValidVelocity(val)	validVelocityRange().includes( val, false )
#define mIsValidImpedance(val)	validImpedanceRange().includes( val, false )


/*!\brief Acoustic Impedance layer. */

mExpClass(Algo) AILayer
{
public:
		AILayer( float thkness, float vel, float den )
		    : thickness_(thkness), vel_(vel), den_(den) {}

		AILayer(float thkness,float ai,float den,
			bool needcompthkness);
		    //!< if den is udf, uses Garner for velocity

    bool	operator ==( const AILayer& p ) const
		{ return thickness_ == p.thickness_
			&& vel_ == p.vel_ && den_ == p.den_; }

    float	thickness_;
    float	vel_;
    float	den_;

    float	getAI() const;
    bool	isOK(bool dodencheck=true) const;
    bool	hasValidVel() const;
    bool	hasValidDen() const;

    bool	fillDenWithVp(bool onlyifinvalid);	//!< uses Gardner

};


mExpClass(Algo) AIModel : public TypeSet<AILayer>
{
public:
		    /* depth of the middle of the layer */
    float	getLayerDepth(int) const;

};



/*!\brief A table of elastic prop layers. */

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
    bool	hasValidVs() const;

    bool	fillVsWithVp(bool onlyifinvalid);	//!< uses Castagna
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
		    /*! Ensures a model does not have layers below a given
		         thickness. last layer may not comply though */
    void	upscale(float maxthickness);
		    /*! Smashes every consecutive set of nblock layers
			into one output layer */
    void	upscaleByN(int nblock);
		    /*! Ensures that all layers are not thicker than a maximum
		        thickness. Splits the blocks if necessary */
    void	setMaxThickness(float maxthickness);
		    /*! Merges consecutive layers with same properties. */
    void	mergeSameLayers();
		    /*! Block elastic model so that no blocks have larger
		        difference than the threshold. Attempts will be made
			to put boundaries at large changes.
		        \param pvelonly use density and SVel as well if false */
    void	block(float relthreshold,bool pvelonly);

		    /*! calculate an upscaled elastic layer using simple
		      weighted averaging.
		      The thickness of the input and output remains constant.
		      returns false if the input model does not contain a single
		      valid input layer */
    bool	getUpscaledByThicknessAvg(ElasticLayer& outlay) const;
		    /* calculate an upscaled elastic layer using the Backus
		       upscaling method. The thickness of the input and
		       output remains constant.
		       returns false if the input model does not contain
		       a single valid input layer
		       \param theta Incidence angle in radians */
    bool	getUpscaledBackus(ElasticLayer& outlay,float theta=0.) const;

    bool	createFromVel(const StepInterval<float>& zrange,
			      const float* pvel, const float* svel=0,
			      const float* den=0);
    bool	createFromAI(const StepInterval<float>& zrange,const float* ai,
			     const float* si =0,const float* den =0);

		    /* depth of the middle of the layer */
    float	getLayerDepth(int layeridx) const;

    Interval<float> getTimeRange(bool usevs=false) const;

protected:

		    /*! Calculates first derivative of the elastic properties
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


mExpClass(Algo) ElasticModelSet : public ManagedObjectSet<ElasticModel>
{
public:

    void		setSize(size_type);
    Interval<float>	getTimeRange(bool usevs=false) const;

};
