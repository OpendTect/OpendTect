#ifndef wellloginterpolator_h
#define wellloginterpolator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		Sept 2010
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"

#include "enums.h"
#include "dbkey.h"
class BufferStringSet;
class Gridder2D;
class InterpolationLayerModel;
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

    void			setWellData(const TypeSet<DBKey>&,
					    const char* lognm);
    void			getWellNames(BufferStringSet&) const;
    void			getWellIDs(TypeSet<DBKey>&) const;
    const char*			getLogName() const;

    void			setGridder(const char* nm,float radius=0);
    const char*			getGridderName() const;
    float			getSearchRadius() const;

    void			setLayerModel(InterpolationLayerModel*);
    const InterpolationLayerModel* getLayerModel() const;

    enum ExtensionModel		{ None, EdgeValueOnly, ExtrapolateEdgeValue };
				mDeclareEnumUtils(ExtensionModel)
    ExtensionModel		extensionMethod() const	{ return extension_; }
    void			extensionMethod(ExtensionModel ext)
				{ extension_ = ext; }

    bool			useLogExtension() const  { return extlog_; }
    void			useLogExtension(bool yn) { extlog_ = yn; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    virtual uiString		errMsg() const		{ return errmsg_; }

    virtual bool		canInputAndOutputBeSame() const { return true; }
    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		needsInput() const		{ return false;}
    virtual bool		prefersBinIDWise() const	{ return true; }

protected:

    virtual bool		prepareComp(int);
    virtual bool		computeBinID(const BinID&,int);
    virtual od_int64		extraMemoryUsage(OutputSlotID,
					const TrcKeySampling&,
					const StepInterval<int>&) const;

    InterpolationLayerModel*	layermodel_;
    Gridder2D*			gridder_;
    ObjectSet<WellLogInfo>	infos_;
    TypeSet<DBKey>		wellmids_;
    BufferString		logname_;

    ExtensionModel		extension_;
    bool			extlog_;

    uiString			errmsg_;
    StepInterval<int>		outputinlrg_;
    StepInterval<int>		outputcrlrg_;

};

} // namespace VolProc

#endif
