#ifndef wellloginterpolator_h
#define wellloginterpolator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		Sept 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "arrayndalgo.h"
#include "multiid.h"
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
    void			releaseData();

    bool			is2D() const;

    void			getWellNames(BufferStringSet&) const;
    void			getWellIDs(TypeSet<MultiID>&) const;
    const char*			getLogName() const;
    const char*			getGridderName() const;
    float			getSearchRadius() const;
    const InterpolationLayerModel* getLayerModel() const;
    Well::ExtractParams	getWellExtractParams() { return params_; }

    void			setGridder(const char* nm,float radius=0);
    void			setWellData(const TypeSet<MultiID>&,
					    const char* lognm);
    void			setWellExtractParams(Well::ExtractParams params)
				{ params_ = params;}
    void			setLayerModel(InterpolationLayerModel*);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    virtual uiString		errMsg() const	{ return errmsg_; }

    bool			canInputAndOutputBeSame() const { return true; }
    bool			needsFullVolume() const		{ return false;}
    bool			needsInput() const		{ return false;}

    /* mDeprecated (this function will be protected virtual after 6.2) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int);
    bool			computeBinID(const BinID&,int);

    InterpolationLayerModel*	layermodel_;
    Gridder2D*			gridder_;
    InverseDistanceGridder2D*	invdistgridder_;
    PolyTrend::Order		trendorder_;
    ObjectSet<WellLogInfo>	infos_;
    TypeSet<MultiID>		wellmids_;
    BufferString		logname_;
    bool			doinverse_;

    StepInterval<int>		outputinlrg_;
    StepInterval<int>		outputcrlrg_;
    Well::ExtractParams		params_;
};

} // namespace VolProc

#endif
