#ifndef emmarchingcubessurface_h
#define emmarchingcubessurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmarchingcubessurface.h,v 1.4 2009-02-02 21:52:02 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "emobject.h"
#include "embody.h"
#include "samplingdata.h"

class MarchingCubesSurface;

namespace EM
{

mClass MarchingCubesSurface : public Body, public EMObject
{ mDefineEMObjFuncs( MarchingCubesSurface );
public:

    virtual int			nrSections() const 		{ return 1; }
    virtual SectionID		sectionID(int) const		{ return 0; }
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
    const ::MarchingCubesSurface&surface() const { return *mcsurface_; }
    bool			setSurface(::MarchingCubesSurface*);

    const SamplingData<int>&	inlSampling() const	{ return inlsampling_; }
    const SamplingData<int>&	crlSampling() const	{ return crlsampling_; }
    const SamplingData<float>&	zSampling() const	{ return zsampling_; }
    void			setInlSampling(const SamplingData<int>&);
    void			setCrlSampling(const SamplingData<int>&);
    void			setZSampling(const SamplingData<float>&);

    ImplicitBody*		createImplicitBody(TaskRunner*) const;

protected:

    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    SamplingData<float>		zsampling_;
    ::MarchingCubesSurface*	mcsurface_;
};


}; // Namespace

#endif
