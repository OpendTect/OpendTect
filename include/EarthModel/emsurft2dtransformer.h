#pragma once
/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "typeset.h"
#include "objectset.h"

class ZAxisTransform;
namespace Geometry { class FaultStick; class FaultStickSet; }

namespace EM
{

class Horizon3D;
class EMObject;
class FaultStickSet;
class FaultStickSetGeometry;

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

    static Executor*		createExecutor(
				const ObjectSet<SurfaceT2DTransfData>& datas,
				ZAxisTransform&,ObjectType);

				~SurfaceT2DTransformer();

    uiString			uiMessage() const override  { return msg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override     { return nrdone_; }
    od_int64			totalNr() const override;

    inline uiRetVal		errMsg() const { return errmsg_; }
    RefMan<EMObject>		getTransformedSurface(const MultiID&) const;

protected:
				SurfaceT2DTransformer(
				const ObjectSet<SurfaceT2DTransfData>& datas,
				ZAxisTransform&);
    virtual void		preStepCB(CallBacker*);
    virtual void		postStepCB(CallBacker*);

    virtual OD::Pol2D3D		dataTypeSupported() { return OD::Both2DAnd3D; }
    bool			do3DHorizon(const EM::EMObject&,Surface&);

    void			load3DTranformVol(const TrcKeyZSampling*);
    bool			load2DVelCubeTransf(const Pos::GeomID&,
					    const StepInterval<int>&);
    void			unloadModel();
    inline virtual bool		updateHSamp() const { return false; }

    virtual const StringView	getTypeString() const	    = 0;

    RefMan<EMObject>		createObject(const MultiID&,
							const MultiID&) const;

    od_int64					nrdone_     = 0;
    od_int64					totnr_	    = 0;
    int						zatvoi_     = -1;
    uiRetVal					errmsg_;
    uiString					msg_;
    ZAxisTransform&				zatf_;
    const ObjectSet<SurfaceT2DTransfData>&	datas_;
    RefObjectSet<EMObject>			outsurfs_;
};


mExpClass(EarthModel) Horizon3DT2DTransformer : public SurfaceT2DTransformer
{ mODTextTranslationClass(Horizon3DT2DTransformer)
public:
			    Horizon3DT2DTransformer(
				const ObjectSet<SurfaceT2DTransfData>&,
				ZAxisTransform&);
			    ~Horizon3DT2DTransformer();
protected:

    OD::Pol2D3D		    dataTypeSupported() override { return OD::Only3D; }
    void		    preStepCB(CallBacker*) override;
    void		    postStepCB(CallBacker*) override;
    int			    nextStep() override;
    const StringView	    getTypeString() const override;
    inline virtual bool     updateHSamp() const override { return true; }
    bool		    doHorizon(const SurfaceT2DTransfData&);

    int			    zatvoi_	= -1;
};


mExpClass(EarthModel) Horizon2DDataHolder
{
public:
				Horizon2DDataHolder(const Pos::GeomID&);
    Horizon2DDataHolder&	operator =(const Horizon2DDataHolder&);
    bool			operator ==(const Horizon2DDataHolder&) const;
    void			addHorMID(const MultiID&);
    const Pos::GeomID&		getGeomID() const;
    const TypeSet<MultiID>&	getMIDSet() const;
protected:
    TypeSet<MultiID>		mids_;
    Pos::GeomID			geomid_;
};


mExpClass(EarthModel) Horizon2DDataHolderSet :
					    public TypeSet<Horizon2DDataHolder>
{
public:
				    Horizon2DDataHolderSet();
    bool			    positionExists(const Pos::GeomID&);
    void			    addData(const Pos::GeomID&,const MultiID&);
};


mExpClass(EarthModel) Horizon2DT2DTransformer : public SurfaceT2DTransformer
{ mODTextTranslationClass(Horizon2DT2DTransformer)
public:
			    Horizon2DT2DTransformer(
				const ObjectSet<SurfaceT2DTransfData>&,
				ZAxisTransform&);
			    ~Horizon2DT2DTransformer();
protected:

    OD::Pol2D3D		    dataTypeSupported() override { return OD::Only2D; }
    void		    preStepCB(CallBacker*) override;
    int			    nextStep() override;
    const StringView	    getTypeString() const override;
    bool		    do2DHorizon(const Horizon2DDataHolder&);
private:
    TypeSet<Pos::GeomID>	    geomids_;
    Horizon2DDataHolderSet	    dataset_;
    RefObjectSet<const Surface>     inpsurfs_;
    TypeSet<MultiID>		    preprocessesedinpmid_;
};


mExpClass(EarthModel) FaultT2DTransformer : public SurfaceT2DTransformer
{ mODTextTranslationClass(FaultT2DTransformer)
public:
			    FaultT2DTransformer(
				const ObjectSet<SurfaceT2DTransfData>&,
				ZAxisTransform&);
			    ~FaultT2DTransformer();
protected:

    OD::Pol2D3D		    dataTypeSupported() override { return OD::Only3D; }
    int			    nextStep() override;
    bool		    doFault(const SurfaceT2DTransfData&);
    const StringView	    getTypeString() const override;
};


mExpClass(EarthModel) FaultSetT2DTransformer : public SurfaceT2DTransformer
{ mODTextTranslationClass(FaultSetT2DTransformer)
public:
				FaultSetT2DTransformer(
				    const ObjectSet<SurfaceT2DTransfData>&,
				    ZAxisTransform&);
				~FaultSetT2DTransformer();
protected:

    OD::Pol2D3D		    dataTypeSupported() override { return OD::Only3D; }
    int			    nextStep() override;
    bool		    doFaultSet(const SurfaceT2DTransfData&);
    const StringView	    getTypeString() const override;
};


mExpClass(EarthModel) FaultStickSetT2DTransformer :
					    public SurfaceT2DTransformer
{ mODTextTranslationClass(FaultSetT2DTransformer)
public:
				FaultStickSetT2DTransformer(
				    const ObjectSet<SurfaceT2DTransfData>&,
				    ZAxisTransform&,bool is2d);
				~FaultStickSetT2DTransformer();
protected:

    bool		    is2d_;

    OD::Pol2D3D		    dataTypeSupported() override
						{ return OD::Both2DAnd3D; }
    int			    nextStep() override;
    bool		    doFaultStickSet(const SurfaceT2DTransfData&);
    const StringView	    getTypeString() const override;

    bool	    handle2DTransformation(const EM::FaultStickSetGeometry&,
						EM::FaultStickSet&);
    bool	    handle3DTransformation(const EM::FaultStickSetGeometry&,
						EM::FaultStickSet&);
    bool	    doTransformation(const Geometry::FaultStick*,int,
						EM::FaultStickSet&,
						Pos::GeomID=Pos::GeomID::udf());

};
}
