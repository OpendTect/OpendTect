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

    const char*			type() const		{ return typeStr(); }
    virtual int			nrSections() const	{ return 1; }
    virtual SectionID		sectionID(int) const	{ return 0; }
    virtual bool		canSetSectionName() const	{ return false;}

    Geometry::Element*		sectionGeometry(const SectionID&) { return 0; }
    const Geometry::Element*	sectionGeometry(const SectionID&) const
								  { return 0; }
    virtual Executor*		loader();
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*);
    virtual bool		isEmpty() const;

    const IOObjContext&		getIOObjContext() const;

    ::MarchingCubesSurface&	surface() { return *mcsurface_; }
    const ::MarchingCubesSurface& surface() const { return *mcsurface_; }
    bool			setSurface(::MarchingCubesSurface*);

    const SamplingData<int>&	inlSampling() const	{ return inlsampling_; }
    const SamplingData<int>&	crlSampling() const	{ return crlsampling_; }
    const SamplingData<float>&	zSampling() const	{ return zsampling_; }
    void			setInlSampling(const SamplingData<int>&);
    void			setCrlSampling(const SamplingData<int>&);
    void			setZSampling(const SamplingData<float>&);

    ImplicitBody*		createImplicitBody(TaskRunner*,bool) const;
    bool			getBodyRange(TrcKeyZSampling& cs);

    void			refBody();
    void			unRefBody();
    MultiID			storageID() const;
    BufferString		storageName() const;

    EM::BodyOperator*		getBodyOperator() const	{ return operator_; }
    void			createBodyOperator();
    void			setBodyOperator(EM::BodyOperator*);
				/*<Set operator only, to use it, call
				   regenerateMCBody() to update the surface.*/
    bool			regenerateMCBody(TaskRunner* tr=0);

    bool			useBodyPar(const IOPar&);
    void			fillBodyPar(IOPar&) const;

    uiString			getUserTypeStr() const
				{ return tr("Marching Cubes Geobody"); }

protected:

    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    SamplingData<float>		zsampling_;
    ::MarchingCubesSurface*	mcsurface_;
    EM::BodyOperator*		operator_;
};

} // namespace EM

