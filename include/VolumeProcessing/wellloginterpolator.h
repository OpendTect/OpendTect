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

#include "enums.h"
#include "multiid.h"
#include "volprocchain.h"

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
{
public:
				mDefaultFactoryInstantiation( Step,
				    WellLogInterpolator,
				    "WellLog Interpolator",
				    sFactoryKeyword() )

				WellLogInterpolator();
				~WellLogInterpolator();

    bool			needsInput() const	{ return false;}

    bool			is2D() const;

    void			setWellData(const TypeSet<MultiID>&,
					    const char* lognm);
    void			getWellNames(BufferStringSet&) const;
    void			getWellIDs(TypeSet<MultiID>&) const;
    const char*			getLogName() const;

    void			setGridder(const char* nm,float radius=0);
    const char*			getGridderName() const;
    float			getSearchRadius() const;

    bool			setLayerModel(const IOPar&);
    bool			getLayerModel(IOPar&) const;

    enum ExtensionModel		{ None, EdgeValueOnly, ExtrapolateEdgeValue };
				DeclareEnumUtils(ExtensionModel)
    ExtensionModel		extensionMethod() const	{ return extension_; }
    void			extensionMethod(ExtensionModel ext)
				{ extension_ = ext; }

    bool			useLogExtension() const  { return extlog_; }
    void			useLogExtension(bool yn) { extlog_ = yn; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			releaseData();
    bool			canInputAndOutputBeSame() const { return true; }
    bool			needsFullVolume() const		{ return false;}

    uiStringCopy		errMsg() const	{ return errmsg_; }

protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int);
    bool			computeBinID(const BinID&,int);

    InterpolationLayerModel*	layermodel_;
    Gridder2D*			gridder_;
    ObjectSet<WellLogInfo>	infos_;
    TypeSet<MultiID>		wellmids_;
    BufferString		logname_;

    ExtensionModel		extension_;
    bool			extlog_;

    uiString			errmsg_;
    StepInterval<int>		outputinlrg_;
    StepInterval<int>		outputcrlrg_;
};

} // namespace VolProc

#endif
