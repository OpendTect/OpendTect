#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/


#include "algomod.h"
#include "factory.h"
#include "survgeom.h"
#include "typeset.h"

class TaskRunnerProvider;
class TrcKey;
class TrcKeySampling;
class TrcKeyZSampling;


mExpClass(Algo) InterpolationLayerModel
{
public:
			mDefineFactoryInClass(InterpolationLayerModel,factory)
    virtual		~InterpolationLayerModel();

    virtual InterpolationLayerModel* clone() const			= 0;

    virtual bool	isOK(const TrcKey* tk=0) const;

    virtual bool	prepare(const TrcKeyZSampling&,
				const TaskRunnerProvider&);
    virtual void	addSampling(const TrcKeySampling&);
    virtual od_int64	getMemoryUsage(const TrcKeySampling&) const;
			/*!< returns total amount of bytes needed to store
			     the Z values of the model */

    virtual float	getLayerIndex(const TrcKey&,float z) const	= 0;
    virtual float	getInterpolatedZ(const TrcKey&,float layer) const;

    static const char*	sKeyModelType();

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)		{ return true; }

protected:
			InterpolationLayerModel();
			InterpolationLayerModel(const InterpolationLayerModel&);

    virtual bool	hasSampling() const;

    ObjectSet<TrcKeySampling>	tkss_;
    ZSampling		zsamp_;

private:

    virtual int		nrLayers() const				= 0;
    virtual float	getZ(const TrcKey&,int layer) const		= 0;
};


mExpClass(Algo) ZSliceInterpolationModel : public InterpolationLayerModel
{ mODTextTranslationClass(ZSliceInterpolationModel);
public:
			mDefaultFactoryInstantiation(
				InterpolationLayerModel,
				ZSliceInterpolationModel,
				"ZSlices",
				tr("Z Slices"))

protected:
			ZSliceInterpolationModel();
			ZSliceInterpolationModel(
					const ZSliceInterpolationModel&);

private:

    InterpolationLayerModel*	clone() const;

    bool		isOK(const TrcKey* tk=0) const;

    int			nrLayers() const;
    float		getLayerIndex(const TrcKey&,float z) const;
    float		getZ(const TrcKey&,int layer) const;
    od_int64		getMemoryUsage(const TrcKeySampling&) const
			{ return 0; }

};
