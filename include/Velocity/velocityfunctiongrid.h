#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"

#include "arrayndalgo.h"
#include "binnedvalueset.h"
#include "samplingdata.h"
#include "thread.h"
#include "notify.h"
#include "velocityfunction.h"

class BinnedValueSet;
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

    StepInterval<float>		getAvailableZ() const;
    bool			moveTo(const BinID&);
    bool			fetchSources();

    bool			isInfluencedBy(const BinID&) const;

    Gridder2D*			getGridder() { return gridder_; }
    void			setGridder(const Gridder2D&); //!<I will clone
    void			setTrendOrder(PolyTrend::Order);
    void			setLayerModel(const InterpolationLayerModel*);

protected:
				~GriddedFunction();
    friend			class GriddedSource;

    bool			computeVelocity(float z0, float dz, int nr,
					float* res ) const;
    ConstRefMan<Function>	getInputFunction(const BinID& bid,int& source);
    void			fetchPerfectFit(const BinID&);

    ObjectSet<const Function>	velocityfunctions_;
    TypeSet<int>		sources_;
    const Function*		directsource_;

    Gridder2D*			gridder_;
    PolyTrend::Order		trendorder_;
    const InterpolationLayerModel* layermodel_;

    mutable TypeSet<float>	gridvalues_;
};


mExpClass(Velocity) GriddedSource : public FunctionSource
{
public:
				GriddedSource();
    const VelocityDesc&		getDesc() const;
    const char*			factoryKeyword() const { return sType(); }
    static const char*		sType() { return "GridVelocity"; }

    const Gridder2D*		getGridder() const;
    void			setGridder(Gridder2D*); //!<Becomes mine
    void			setTrendOrder(PolyTrend::Order);

    void			setSource(ObjectSet<FunctionSource>&);
    void			setSource(const DBKeySet&);
    void			getSources(DBKeySet&) const;

    void			setLayerModel(const InterpolationLayerModel*);

    const ObjectSet<FunctionSource>&	getSources() const;

    NotifierAccess*		changeNotifier() { return &notifier_; }
    BinID			changeBinID() const { return changebid_; }

    GriddedFunction*		createFunction();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    friend			class GriddedFunction;
    GriddedFunction*		createFunction(const BinID&);
				~GriddedSource();
    bool			initGridder();

    void			sourceChangeCB(CallBacker*);

    ObjectSet<FunctionSource>	datasources_;

    Notifier<GriddedSource>	notifier_;
    BinID			changebid_;
    Gridder2D*			gridder_;
    PolyTrend::Order		trendorder_;
    bool			gridderinited_;
    const InterpolationLayerModel* layermodel_;

    BinnedValueSet		sourcepos_;		//All sources

    TypeSet<BinID>		gridsourcebids_;	//Filtered
    TypeSet<Coord>		gridsourcecoords_;	//Filtered
};

} // namespace Vel
