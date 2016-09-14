#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
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

/*!\brief VolProc::Step for velocity gridding. */

mExpClass(VolumeProcessing) VelocityGridder : public Step
{ mODTextTranslationClass(VelocityGridder)
public:
			mDefaultFactoryInstantiation(
				Step, VelocityGridder,
				"Gridding", tr("Velocity gridder") )

			VelocityGridder();
			~VelocityGridder();
    virtual void	releaseData();

    void		setSources(ObjectSet<Vel::FunctionSource>&);
    const ObjectSet<Vel::FunctionSource>&	getSources() const;

    void		setGridder(Gridder2D*); //becomes mine
    const Gridder2D*	getGridder() const;

    void		setLayerModel(InterpolationLayerModel*); //becomes mine
    const InterpolationLayerModel* getLayerModel() const;

    virtual const VelocityDesc* getVelDesc() const;
    virtual uiString	errMsg() const			{ return errmsg_; }
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    virtual bool	needsInput() const;
    virtual bool	canInputAndOutputBeSame() const	{ return true; }
    virtual bool	needsFullVolume() const		{ return true;}
    virtual bool	areSamplesIndependent() const	{ return true; }

    static const char*	sKeyType()			{ return "Type"; }
    static const char*	sKeyID()			{ return "ID"; }
    static const char*	sKeyNrSources()			{ return "NrSources"; }
    static const char*	sKeyGridder()			{ return "Gridder"; }

protected:

    virtual Task*	createTask();
    virtual od_int64	extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

    InterpolationLayerModel*		layermodel_;
    Gridder2D*				gridder_;
    ObjectSet<Vel::FunctionSource>	sources_;
    uiString				errmsg_;

};

} // namespace VolProc

