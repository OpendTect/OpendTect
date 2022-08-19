#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "veldesc.h"

class Gridder2D;
class InterpolationLayerModel;
namespace Vel
{
    class Function;
    class FunctionSource;
}


namespace VolProc
{

/*!
\brief VolProc::Step for velocity gridding.
*/

mExpClass(VolumeProcessing) VelocityGridder : public Step
{ mODTextTranslationClass(VelocityGridder)
public:
			mDefaultFactoryInstantiation(
				Step, VelocityGridder,
				"Gridding", tr("Velocity gridder") )

			VelocityGridder();
			~VelocityGridder();

    const VelocityDesc* getVelDesc() const override;

    void		setSources(ObjectSet<Vel::FunctionSource>&);
    const ObjectSet<Vel::FunctionSource>&	getSources() const;

    void		setGridder(Gridder2D*); //becomes mine
    const Gridder2D*	getGridder() const;

    void		setLayerModel(InterpolationLayerModel*); //becomes mine
    const InterpolationLayerModel* getLayerModel() const;

    bool		needsInput() const override;
    void		releaseData() override;
    bool		canInputAndOutputBeSame() const override
			{ return true; }

    bool		needsFullVolume() const override	{ return true;}

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    virtual uiString	errMsg() const override		{ return errmsg_; }

    static const char*	sKeyType()		{ return "Type"; }
    static const char*	sKeyID()		{ return "ID"; }
    static const char*	sKeyNrSources()		{ return "NrSources"; }
    static const char*	sKeyGridder()		{ return "Gridder"; }

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    Task*		createTask() override;

    InterpolationLayerModel*		layermodel_;
    Gridder2D*				gridder_;
    ObjectSet<Vel::FunctionSource>	sources_;
};

} // namespace VolProc
