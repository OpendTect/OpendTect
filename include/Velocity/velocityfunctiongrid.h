#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "binidvalset.h"
#include "thread.h"
#include "velocityfunction.h"

class BinIDValueSet;
class Gridder2D;
class InterpolationLayerModel;

namespace Vel
{

class GriddedSource;

/*!A velocity funcion where the velocity is computed from
   Residual Moveout picks. */

mExpClass(Velocity) GriddedFunction : public Function
{
public:
				GriddedFunction(GriddedSource&);

    ZSampling			getAvailableZ() const override;
    bool			moveTo(const BinID&) override;
    bool			fetchSources();

    bool			isInfluencedBy(const BinID&) const;

    void			setGridder(const Gridder2D&); //!<I will clone
    Gridder2D*			getGridder() { return gridder_; }
    void			setLayerModel(const InterpolationLayerModel*);

protected:
				~GriddedFunction();

    friend			class GriddedSource;

    bool			computeVelocity(float z0,float dz,int sz,
						float* res) const override;

    ConstRefMan<Function>	getInputFunction(const BinID& bid,int& source);
    void			fetchPerfectFit(const BinID&);

    ObjectSet<const Function>	velocityfunctions_;
    TypeSet<int>		sources_;
    const Function*		directsource_			= nullptr;

    Gridder2D*			gridder_			= nullptr;
    const InterpolationLayerModel* layermodel_			= nullptr;

    mutable TypeSet<float>	gridvalues_;
};


mExpClass(Velocity) GriddedSource : public FunctionSource
{
public:
				GriddedSource(const Pos::GeomID&);

    const VelocityDesc&		getDesc() const override;
    const ZDomain::Info&	zDomain() const override;

    const char*			factoryKeyword() const override
				{ return sType(); }
    static const char*		sType() { return "GridVelocity"; }

    const Gridder2D*		getGridder() const;
    void			setGridder(Gridder2D*); //!<Becomes mine

    void			setSource(ObjectSet<FunctionSource>&);
    void			setSource(const TypeSet<MultiID>&);
    void			getSources(TypeSet<MultiID>&) const;

    void			setLayerModel(const InterpolationLayerModel*);
    FunctionSource&		setZDomain(const ZDomain::Info&) override;

    const ObjectSet<FunctionSource>& getSources() const;

    NotifierAccess*		changeNotifier() override { return &notifier_; }
    BinID			changeBinID() const override
				{ return changebid_; }

    GriddedFunction*		createFunction();

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
				~GriddedSource();

    friend			class GriddedFunction;
    GriddedFunction*		createFunction(const BinID&) override;
    bool			initGridder();
    static const char*		sKeyGridder() { return "Gridder"; }

    void			sourceChangeCB(CallBacker*);

    ObjectSet<FunctionSource>	datasources_;

    Notifier<GriddedSource>	notifier_;
    BinID			changebid_;
    Gridder2D*			gridder_;
    bool			gridderinited_		= false;
    const InterpolationLayerModel* layermodel_		= nullptr;
    const Pos::GeomID		geomid_;

    BinIDValueSet		sourcepos_;		//All sources

    TypeSet<BinID>		gridsourcebids_;	//Filtered
    TypeSet<Coord>		gridsourcecoords_;	//Filtered
};

} // namespace Vel
