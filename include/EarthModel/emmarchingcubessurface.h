#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emobject.h"
#include "embody.h"
#include "samplingdata.h"

class MarchingCubesSurface;

namespace EM
{

class BodyOperator;

/*!
\brief Marching cubes surface
*/

mExpClass(EarthModel) MarchingCubesSurface : public Body, public EMObject
{ mDefineEMObjFuncs( MarchingCubesSurface );
public:

    const char*			type() const override	{ return typeStr(); }

    Executor*			loader() override;
    Executor*			saver() override;
    virtual Executor*		saver(const IOObj*);
    bool			isEmpty() const override;

    const IOObjContext&		getIOObjContext() const override;

    ::MarchingCubesSurface&	surface() { return *mcsurface_; }
    const ::MarchingCubesSurface& surface() const { return *mcsurface_; }
    bool			setSurface(::MarchingCubesSurface*);

    const SamplingData<int>&	inlSampling() const	{ return inlsampling_; }
    const SamplingData<int>&	crlSampling() const	{ return crlsampling_; }
    const SamplingData<float>&	zSampling() const	{ return zsampling_; }
    void			setInlSampling(const SamplingData<int>&);
    void			setCrlSampling(const SamplingData<int>&);
    void			setZSampling(const SamplingData<float>&);
    void			setSampling(const TrcKeyZSampling&);

    ImplicitBody*		createImplicitBody(TaskRunner*,
						   bool) const override;
    bool			getBodyRange(TrcKeyZSampling& cs) override;

    void			refBody() override;
    void			unRefBody() override;
    MultiID			storageID() const override;
    BufferString		storageName() const override;

    EM::BodyOperator*		getBodyOperator() const { return operator_; }
    void			createBodyOperator();
    void			setBodyOperator(EM::BodyOperator*);
				/*<Set operator only, to use it, call
				   regenerateMCBody() to update the surface.*/
    bool			regenerateMCBody(TaskRunner* tr=0);

    bool			useBodyPar(const IOPar&) override;
    void			fillBodyPar(IOPar&) const override;

    uiString			getUserTypeStr() const override
				{ return tr("Marching Cubes Geobody"); }

protected:

    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    SamplingData<float>		zsampling_;
    ::MarchingCubesSurface*	mcsurface_;
    EM::BodyOperator*		operator_;
};

} // namespace EM
