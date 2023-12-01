#pragma once
/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "objectset.h"

class ZAxisTransform;

namespace EM
{

class Horizon3D;
class EMObject;

mExpClass(EarthModel) SurfaceT2DTransfData
{
public:
			    SurfaceT2DTransfData(const SurfaceIOData&);
			    mOD_DisableCopy(SurfaceT2DTransfData);

    MultiID			inpmid_;
    MultiID			outmid_;
    SurfaceIODataSelection	surfsel_;
};


mExpClass(EarthModel) SurfaceT2DTransformer : public Executor
{ mODTextTranslationClass(SurfaceT2DTransformer)
public:
			SurfaceT2DTransformer(
			    const ObjectSet<SurfaceT2DTransfData>& datas,
			    ZAxisTransform&,IOObjInfo::ObjectType);
			~SurfaceT2DTransformer();

    uiString			uiMessage() const override  { return curmsg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override     { return nrdone_; }
    od_int64			totalNr() const override;

    void			setZDomain(const ZDomain::Info&);

    RefMan<Surface>		getTransformedSurface(const MultiID&) const;

private:
    void			preStepCB(CallBacker*);
    int				nextStep() override;
    void			postStepCB(CallBacker*);

    bool			doHorizon(const SurfaceT2DTransfData&);
    bool			do3DHorizon(const EM::EMObject&,Surface&);
    bool			do2DHorizon(const EM::EMObject&,
					const SurfaceT2DTransfData&,Surface&);

    bool			load2DVelCubeTransf(const Pos::GeomID&,
						const StepInterval<int>&);
    void			unload2DVelCubeTransf();
    void			unloadVolume();

    bool			is2DObject() const;
    bool			is3DObject() const;

    RefMan<Surface>		createSurface(const MultiID&);

    od_int64					nrdone_		= 0;
    od_int64					totnr_		= 0;
    uiString					curmsg_;
    ZAxisTransform&				zatf_;
    const ObjectSet<SurfaceT2DTransfData>&	datas_;
    const ZDomain::Info*			zinfo_;
    const IOObjInfo::ObjectType			objtype_;
    int						zatvoi_		= -1;
    RefObjectSet<Surface>			surfs_;
};

} // namespace EM
