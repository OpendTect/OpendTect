#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "trckeyzsampling.h"
#include "factory.h"
#include "typeset.h"

class TaskRunner;

mExpClass(Algo) InterpolationLayerModel
{
public:
			mDefineFactoryInClass(InterpolationLayerModel,factory)
    virtual		~InterpolationLayerModel();

    virtual InterpolationLayerModel* clone() const			= 0;

    virtual bool	isOK(const TrcKey* tk=0) const;

    virtual bool	prepare(const TrcKeyZSampling&,TaskRunner* =0);
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
    StepInterval<float> zsamp_;

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

    InterpolationLayerModel*	clone() const override;

    bool		isOK(const TrcKey* =nullptr) const override;

    int			nrLayers() const override;
    float		getLayerIndex(const TrcKey&,float z) const override;
    float		getZ(const TrcKey&,int layer) const override;
    od_int64		getMemoryUsage(const TrcKeySampling&) const override
			{ return 0; }

};
