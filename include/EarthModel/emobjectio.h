#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		6-01-2017
________________________________________________________________________


-*/

#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emfaultstickset.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "factory.h"
#include "dbkey.h"
#include "saveable.h"
#include "uistrings.h"

class Executor;
class TaskRunnerProvider;

namespace EM
{

class Object;
class SurfaceIODataSelection;

mExpClass(EarthModel) ObjectLoader : public CallBacker
{
public:

    virtual		~ObjectLoader()		{}

    mDefineFactory2ParamInClass(ObjectLoader,const DBKeySet&,
				const SurfaceIODataSelection*,factory)

    virtual uiString	userName()		= 0;
    virtual bool	load(const TaskRunnerProvider&);

    RefObjectSet<Object> getLoadedEMObjects() const { return emobjects_; }
    const DBKeySet&	tobeLodedKeys() const { return dbkeys_; }
    const DBKeySet&	notLoadedKeys() const { return notloadedkeys_; }
    virtual bool	allOK() const
			{ return notloadedkeys_.isEmpty(); }

protected:

			ObjectLoader(const DBKeySet&,
				     const SurfaceIODataSelection*);

    virtual void	addObject( Object* obj ) { emobjects_ += obj; }
    virtual Executor*	createLoaderExec();

    DBKeySet		dbkeys_;
    const SurfaceIODataSelection* sel_;
    DBKeySet		notloadedkeys_;
    RefObjectSet<Object> emobjects_;

    friend class	ObjectLoaderExec;
    friend class	ObjectManager;
    friend class	StoredObjAccessData;

private:

    Executor*		fetchReader(Object*) const;

};


mExpClass(EarthModel) FaultStickSetLoader : public ObjectLoader
{
public:

    mDefaultFactoryInstantiation2Param(ObjectLoader,
				       FaultStickSetLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       EM::FaultStickSet::typeStr(),
				       uiStrings::sFaultStickSet(mPlural))

			FaultStickSetLoader(const DBKeySet&,
					    const SurfaceIODataSelection*);

    uiString		userName() {return uiStrings::sFaultStickSet(mPlural);}

};


mExpClass(EarthModel) Fault3DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation2Param(ObjectLoader,
				       Fault3DLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       Fault3D::typeStr(),
				       uiStrings::sFault(mPlural))

			Fault3DLoader(const DBKeySet&,
				      const SurfaceIODataSelection*);

    uiString		userName() { return uiStrings::sFault(mPlural); }

};


mExpClass( EarthModel ) FaultSet3DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation2Param(ObjectLoader,
				       FaultSet3DLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       FaultSet3D::typeStr(),
				       uiStrings::sFault(mPlural))

			FaultSet3DLoader( const DBKeySet&,
				      const SurfaceIODataSelection* );

    uiString		userName() { return uiStrings::sFaultSet(); }

};


mExpClass(EarthModel) Horizon3DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation2Param(ObjectLoader,
				       Horizon3DLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       Horizon3D::typeStr(),
				       uiStrings::sHorizon(mPlural))

			Horizon3DLoader(const DBKeySet&,
					const SurfaceIODataSelection*);

    uiString		userName() { return uiStrings::sHorizon(mPlural); }

};


mExpClass(EarthModel) Horizon2DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation2Param(ObjectLoader,
				       Horizon2DLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       Horizon2D::typeStr(),
				       uiStrings::s2DHorizon(mPlural))

			Horizon2DLoader(const DBKeySet&,
					const SurfaceIODataSelection*);

    uiString		userName() { return uiStrings::s2DHorizon(mPlural); }

};


mExpClass(EarthModel) BodyLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation2Param(ObjectLoader,
				       BodyLoader,const DBKeySet&,
				       const SurfaceIODataSelection*,
				       sKey::Body(),
				       uiStrings::sBody(mPlural))

			BodyLoader(const DBKeySet&,
				   const SurfaceIODataSelection*);

    uiString		userName() { return uiStrings::sBody(mPlural); }

};


// SAVERS

mExpClass(EarthModel) ObjectSaver : public Saveable
{
public:

    mDefineFactory1ParamInClass(ObjectSaver,const SharedObject&,factory)

			ObjectSaver(const SharedObject&);
			mDeclMonitorableAssignment(ObjectSaver);
			~ObjectSaver();

    ConstRefMan<Object>	emObject() const;
    void		setEMObject(const Object&);

    mDeclInstanceCreatedNotifierAccess(ObjectSaver);

protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) FaultStickSetSaver : public ObjectSaver
{
public:
     mDefaultFactoryInstantiation1Param(ObjectSaver,
				       FaultStickSetSaver,const SharedObject&,
				       FaultStickSet::typeStr(),
				       uiStrings::sFaultStickSet(mPlural))
			FaultStickSetSaver(const SharedObject&);
			~FaultStickSetSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) Fault3DSaver : public ObjectSaver
{
public:
     mDefaultFactoryInstantiation1Param(ObjectSaver,
				       Fault3DSaver,const SharedObject&,
				       Fault3D::typeStr(),
				       uiStrings::sFault(mPlural))
			Fault3DSaver(const SharedObject&);
			~Fault3DSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) FaultSet3DSaver : public ObjectSaver
{
public:
     mDefaultFactoryInstantiation1Param(ObjectSaver,
				       FaultSet3DSaver,const SharedObject&,
				       FaultSet3D::typeStr(),
				       uiStrings::sFaultSet(mPlural))
			FaultSet3DSaver(const SharedObject&);
			~FaultSet3DSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) Horizon3DSaver : public ObjectSaver
{
public:
     mDefaultFactoryInstantiation1Param(ObjectSaver,
				       Horizon3DSaver,const SharedObject&,
				       Horizon3D::typeStr(),
				       uiStrings::sHorizon(mPlural))
			Horizon3DSaver(const SharedObject&);
			~Horizon3DSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) Horizon2DSaver : public ObjectSaver
{
public:
     mDefaultFactoryInstantiation1Param(ObjectSaver,
				       Horizon2DSaver,const SharedObject&,
				       Horizon2D::typeStr(),
				       uiStrings::s2DHorizon(mPlural))
			Horizon2DSaver(const SharedObject&);
			~Horizon2DSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


mExpClass(EarthModel) BodySaver : public ObjectSaver
{
public:
			BodySaver(const SharedObject&);
			~BodySaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;
};


#define mDeclBodySaver(typ,kw) \
mExpClass(EarthModel) typ##BodySaver : public BodySaver \
{ \
public: \
     mDefaultFactoryInstantiation1Param(ObjectSaver, \
				       BodySaver,const SharedObject&, kw, \
				       uiStrings::sBody(mPlural)) \
			typ##BodySaver(const SharedObject& obj) \
			: BodySaver(obj) {} \
}

mDeclBodySaver(MC,MarchingCubesSurface::typeStr());
mDeclBodySaver(Polygon,PolygonBody::typeStr());
mDeclBodySaver(RandomPos,RandomPosBody::typeStr());

} // namespace EM

