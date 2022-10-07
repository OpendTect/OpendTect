#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "enums.h"
#include "math.h"
#include "ranges.h"
#include "survinfo.h"
#include "typeset.h"
#include "manobjectset.h"

template <class T> class Array2D;
class AILayer;
class ElasticLayer;
class VTILayer;
class HTILayer;

mGlobal(Algo) inline float cMinLayerThickness() { return 1e-4f; }
mGlobal(Algo) inline const Interval<float> validThicknessRange();
mGlobal(Algo) inline const Interval<float> validDensityRange();
mGlobal(Algo) inline const Interval<float> validVelocityRange();
mGlobal(Algo) inline const Interval<float> validImpRange();


/*!
\brief Base class for all Acoustic and Elastic impedance layers
*/

mExpClass(Algo) RefLayer
{
public:
    virtual		~RefLayer();

    enum Type		{ Acoustic, Elastic, VTI, HTI };
			mDeclareEnumUtils(Type);

    static RefLayer*	create(Type);
			//<! Uninitialized layer
    static RefLayer*	clone(const RefLayer&,const Type* require =nullptr);
			//<! copies the relevant values, calculates the others

    virtual RefLayer*	clone() const				= 0;
    virtual Type	getType() const				= 0;
    static Type		getType(bool needswave,bool needfracrho,
				bool needfracazi);

    RefLayer&		operator =(const RefLayer&);
    bool		operator ==(const RefLayer&) const;
    bool		operator !=(const RefLayer&) const;

    bool		isOK(bool dodencheck=true,
			     bool dosvelcheck=true,
			     bool dofracrhocheck=true,
			     bool dofracazicheck=true) const;

    virtual float	getThickness() const			= 0;
    virtual float	getPVel() const				= 0;
    virtual float	getDen() const				= 0;
    virtual float	getSVel() const		{ return mUdf(float); }
    virtual float	getFracRho() const	{ return mUdf(float); }
    virtual float	getFracAzi() const	{ return mUdf(float); }
    virtual float	getAI() const				= 0;
    virtual float	getSI() const		{ return mUdf(float); }

    virtual RefLayer&	setThickness(float)			= 0;
    virtual RefLayer&	setPVel(float)				= 0;
    virtual RefLayer&	setDen(float)				= 0;
    virtual RefLayer&	setSVel(float)		{ return *this; }
    virtual RefLayer&	setFracRho(float)	{ return *this; }
    virtual RefLayer&	setFracAzi(float)	{ return *this; }

    virtual bool	isValidThickness() const		= 0;
    virtual bool	isValidVel() const			= 0;
    virtual bool	isValidDen() const			= 0;
    virtual bool	isValidVs() const	{ return false; }
    virtual bool	isValidFraRho() const	{ return false; }
    virtual bool	isValidFracAzi() const	{ return false; }

    virtual bool	isElastic() const	{ return false; }
    virtual bool	isVTI() const		{ return false; }
    virtual bool	isHTI() const		{ return false; }

    virtual const AILayer& asAcoustic() const			= 0;
    virtual const ElasticLayer* asElastic() const { return nullptr; }
    virtual const VTILayer* asVTI() const	{ return nullptr; }
    virtual const HTILayer* asHTI() const	{ return nullptr; }

    virtual AILayer&	asAcoustic()				= 0;
    virtual ElasticLayer* asElastic()		{ return nullptr; }
    virtual VTILayer*	asVTI()			{ return nullptr; }
    virtual HTILayer*	asHTI()			{ return nullptr; }

protected:
			RefLayer();

    virtual void copyFrom(const RefLayer&)			= 0;

};


/*!
\brief Acoustic Impedance layer implementation
*/

mExpClass(Algo) AILayer : public RefLayer
{
public:
		AILayer(float thkness,float vel,float den);
		//Density or velocity will be computed using Gardner equation
		//in case one of them is undef.
		AILayer(float thkness,float ai,float den,
			bool needcompthkness);
		AILayer(const RefLayer&);
		~AILayer();

    RefLayer&	operator =(const AILayer&);
    RefLayer*	clone() const override;
    Type	getType() const override	{ return Acoustic; }

    float	getThickness() const override	{ return thickness_; }
    float	getPVel() const override	{ return vel_; }
    float	getDen() const override		{ return den_; }
    float	getAI() const override;

    RefLayer&	setThickness(float) override;
    RefLayer&	setPVel(float) override;
    RefLayer&	setDen(float) override;
    bool	fillDenWithVp(bool onlyinvalid);
		//<!Compute Den from Gardner

    bool	isValidThickness() const override;
    bool	isValidDen() const override;
    bool	isValidVel() const override;

    const AILayer& asAcoustic() const override		{ return *this; }
    AILayer&	asAcoustic() override			{ return *this; }

protected:

    void	copyFrom(const RefLayer&) override;

private:

    float	thickness_;
    float	vel_;
    float	den_;

};


/*!
\brief Elastic Impedance layer implementation: AI + SVel
*/

mExpClass(Algo) ElasticLayer : public AILayer
{
public:
		ElasticLayer(float thkness,float pvel,float svel,float den);
		ElasticLayer(float thkness,float ai,float si,
			     float den,bool needcompthkness);
		ElasticLayer(const RefLayer&);
		~ElasticLayer();

    RefLayer&	operator =(const ElasticLayer&);
    RefLayer*	clone() const override;
    Type	getType() const override		{ return Elastic; }

    float	getSVel() const override		{ return svel_; }
    float	getSI() const override;

    RefLayer&	setSVel(float) override;
    bool	fillVsWithVp(bool onlyinvalid);
		//!<Compute Vs from Castagna

    bool	isValidVs() const override;

    bool	isElastic() const override		{ return true; }

    const ElasticLayer* asElastic() const override	{ return this; }
    ElasticLayer* asElastic() override			{ return this; }

protected:

    void	copyFrom(const RefLayer&) override;

private:

    float	svel_;
};


/*!\brief A table of elastic prop layers with processing utilities*/
mExpClass(Algo) ElasticModel : public ObjectSet<RefLayer>
{
public:

		ElasticModel();
		ElasticModel(const ObjectSet<RefLayer>&);
		ElasticModel(const ElasticModel&);
		~ElasticModel();

    ElasticModel& operator =(const ElasticModel&);
    ElasticModel& operator -=(RefLayer*) override;
    ElasticModel* clone() const override
		{ return new ElasticModel(*this); }

    bool	isManaged() const override	{ return true; }

    ElasticModel& copyFrom(const ElasticModel& mdl,RefLayer::Type reqtyp);
		/*!< \param mdl input model (can be itself)
		     \param reqtyp return type for all layers in model	 */
    void	append(const ObjectSet<RefLayer>&) override;
    void	erase() override;
    RefLayer*	pop() override;
    RefLayer*	removeSingle(int,bool kporder=true) override;
    void	removeRange(int from,int to) override;
    RefLayer*	replace(int,RefLayer*) override;
    RefLayer*	removeAndTake(int,bool kporder=true);

    RefLayer::Type getType() const;
    RefLayer::Type getMinType() const;
    bool	isElastic() const;
    bool	isVTI() const;
    bool	isHTI() const;

    int		isOK(bool dodencheck=true,bool dosvelcheck=true,
		     bool dofracrhocheck=true,bool dofracazicheck=true) const;
		/*! Checks if all layers have valid property values
		    returns index of first invalid layer */

    bool	getValues(bool isden,bool issvel,TypeSet<float>&) const;
		/*! Get one of the properties */

    bool	getValues(bool vel,bool den,bool svel,
			  Array2D<float>&) const;
		/*! Get several properties, in the order vel - den - svel */

    void	checkAndClean(int& firsterroridx,bool dodencheck=true,
			      bool dosvelcheck=true,bool onlyinvalid=false);

    void	interpolate(bool dovp,bool doden,bool dovs);
		/*! Replaces all undefined or invalid values */

    void	upscale(float maxthickness);
		/*! Ensures a model does not have layers below a given thickness
		  last layer may not comply though */

    void	upscaleByN(int nblock);
		/*! Smashes every consecutive set of nblock layers
		  into one output layer */

    void	setMaxThickness(float maxthickness);
		/*! Ensures that all layers in the elastic model are not thicker
		    than a maximum thickness. Splits the blocks if necessary */

    void	mergeSameLayers();
		/*! Merged consecutive layers with same properties. */

    void	block(float relthreshold,bool pvelonly);
		/*! Block elastic model so that no blocks have larger difference
		  than the threshold. Attempts will be made to put boundaries at
		  large changes.
		  \param relthreshold
		  \param pvelonly Will use density and SVel as well if false */

    bool	getUpscaledByThicknessAvg(RefLayer&) const;
		/*! compute an upscaled elastic layer from an elastic model
		  using simple weighted averaging.
		  The thickness of the input and output remains constant.
		  returns false if the input model does not contain a single
		  valid input layer */

    bool	getUpscaledBackus(RefLayer&,float theta=0.) const;
		/* computes an upscaled elastic layer from an elastic model
		   using backus upscaling method. The thickness of the input and
		   output remains constant.
		   returns false if the input model does not contain a single
		   valid input layer
		   \param theta Incidence angle in radians */

    bool	createFromVel(const ZSampling& zrange,
			      const float* pvel,const float* svel=nullptr,
			      const float* den=nullptr);

    bool	createFromAI(const ZSampling& zrange,const float* ai,
			     const float* si =nullptr,
			     const float* den =nullptr);

    float	getLayerDepth(int layerix) const;
		//<!* Return depth of the middle of the layer

    Interval<float> getTimeSampling(bool usevs=false) const;

private:

    bool	getRatioValues(bool vel,bool den,bool svel,
			       Array2D<float>& ratiovals,
			       Array2D<float>& vals) const;
		/*! Computes first derivative of the elastic properties
		    May also return the input values */

    bool	doBlocking(float threshold,bool pvelonly,
			   TypeSet<Interval<int> >& blocks) const;
		/* Gives layer index distributions of similar properties */

    void	removeSpuriousLayers(float zstep);
		/* If a layer thickness is strictly identical to zstep,
		   maybe split that layer over the last/previous layers */



};


mExpClass(Algo) ElasticModelSet : public ManagedObjectSet<ElasticModel>
{
public:
			ElasticModelSet();
			~ElasticModelSet();

    bool		setSize(int);
    bool		getTimeSampling(Interval<float>&,
					bool usevs=false) const;

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
