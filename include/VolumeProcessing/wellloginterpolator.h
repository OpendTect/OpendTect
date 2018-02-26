#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		Sept 2010
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "arrayndalgo.h"
#include "dbkey.h"
#include "volprocstep.h"
#include "wellextractdata.h"

class BufferStringSet;
class Gridder2D;
class InterpolationLayerModel;
class InverseDistanceGridder2D;
namespace Well { class Data; class Log; }

namespace VolProc
{

class WellLogInfo;
class WellLogInfoSetup;

/*! Fills a volume with well log values. */

mExpClass(VolumeProcessing) WellLogInterpolator : public Step
{ mODTextTranslationClass(WellLogInterpolator)
public:
				mDefaultFactoryInstantiation( Step,
				    WellLogInterpolator,
				    "WellLog Interpolator",
				    tr("WellLog Interpolator") )

				WellLogInterpolator();
				~WellLogInterpolator();
    virtual void		releaseData();

    bool			is2D() const;

    void			getWellNames(BufferStringSet&) const;
    void			getWellIDs(DBKeySet&) const;
    const char*			getLogName() const;
    const Gridder2D*		getGridder() const	{ return gridder_; }
    PolyTrend::Order		getTrendOrder() const	{ return trendorder_; }
    const InterpolationLayerModel* getLayerModel() const;
    const Well::ExtractParams& getWellExtractParams()   { return params_; }

    void			setGridder(const IOPar&);
    void			setWellData(const DBKeySet&,
					    const char* lognm);
    void			setWellExtractParams(
					    const Well::ExtractParams& params )
				{ params_ = params;}
    void			setLayerModel(InterpolationLayerModel*);

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

private:

    virtual bool		canInputAndOutputBeSame() const { return true; }
    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		needsInput() const		{ return false;}
    virtual bool		prefersBinIDWise() const	{ return true; }

    void			setGridder(const Gridder2D*);
    virtual bool		prepareComp(int);
    virtual bool		computeBinID(const BinID&,int);
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						 const TrcKeyZSampling&) const;

    InterpolationLayerModel*	layermodel_;
    Gridder2D*			gridder_;
    InverseDistanceGridder2D*	invdistgridder_;
    PolyTrend::Order		trendorder_;
    ObjectSet<WellLogInfo>	infos_;
    DBKeySet			wellmids_;
    BufferString		logname_;
    Well::ExtractParams		params_;
    bool			doinverse_;

    StepInterval<int>		outputinlrg_;
    StepInterval<int>		outputcrlrg_;
};

} // namespace VolProc
