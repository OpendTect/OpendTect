#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "binidvalset.h"
#include "samplingdata.h"
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

    StepInterval<float>		getAvailableZ() const override;
    bool			moveTo(const BinID&) override;
    bool			fetchSources();

    bool			isInfluencedBy(const BinID&) const;

    void			setGridder(const Gridder2D&); //!<I will clone
    Gridder2D*			getGridder() { return gridder_; }
    void			setLayerModel(const InterpolationLayerModel*);

protected:
				~GriddedFunction();
    friend			class GriddedSource;

    bool			computeVelocity(float z0, float dz, int nr,
					float* res ) const override;
    ConstRefMan<Function>	getInputFunction(const BinID& bid,int& source);
    void			fetchPerfectFit(const BinID&);

    ObjectSet<const Function>	velocityfunctions_;
    TypeSet<int>		sources_;
    const Function*		directsource_;

    Gridder2D*			gridder_;
    const InterpolationLayerModel* layermodel_;

    mutable TypeSet<float>	gridvalues_;
};


mExpClass(Velocity) GriddedSource : public FunctionSource
{
public:
				GriddedSource();
    const VelocityDesc&		getDesc() const override;
    const char*			factoryKeyword() const override
				{ return sType(); }
    static const char*		sType() { return "GridVelocity"; }

    const Gridder2D*		getGridder() const;
    void			setGridder(Gridder2D*); //!<Becomes mine

    void			setSource(ObjectSet<FunctionSource>&);
    void			setSource(const TypeSet<MultiID>&);
    void			getSources(TypeSet<MultiID>&) const;

    void			setLayerModel(const InterpolationLayerModel*);

    const ObjectSet<FunctionSource>&	getSources() const;

    NotifierAccess*		changeNotifier() override { return &notifier_; }
    BinID			changeBinID() const override
				{ return changebid_; }

    GriddedFunction*		createFunction();

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
    friend			class GriddedFunction;
    GriddedFunction*		createFunction(const BinID&) override;
				~GriddedSource();
    bool			initGridder();
    static const char*		sKeyGridder() { return "Gridder"; }

    void			sourceChangeCB(CallBacker*);

    ObjectSet<FunctionSource>	datasources_;

    Notifier<GriddedSource>	notifier_;
    BinID			changebid_;
    Gridder2D*			gridder_;
    bool			gridderinited_;
    const InterpolationLayerModel* layermodel_;

    BinIDValueSet		sourcepos_;		//All sources

    TypeSet<BinID>		gridsourcebids_;	//Filtered
    TypeSet<Coord>		gridsourcecoords_;	//Filtered
};

} // namespace Vel

