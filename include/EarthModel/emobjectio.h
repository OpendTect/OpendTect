#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		6-01-2017
________________________________________________________________________


-*/

#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emhorizon3d.h"
#include "factory.h"
#include "dbkey.h"
#include "saveable.h"
#include "uistrings.h"

class Executor;
class TaskRunner;

namespace EM
{

class Object;
class SurfaceIODataSelection;

mExpClass(EarthModel) ObjectLoader
{
public:

    virtual		~ObjectLoader()		{}

    mDefineFactory2ParamInClass(ObjectLoader,const DBKeySet&,
				const SurfaceIODataSelection*,factory)

    virtual uiString	userName()		= 0;
    virtual bool	load(TaskRunner*)	= 0;
    virtual Executor*	getLoader() const	= 0;

    RefObjectSet<Object> getLoadedEMObjects() const { return emobjects_; }
    const DBKeySet&	tobeLodedKeys() const { return dbkeys_; }
    virtual bool	allOK() const
			{ return notloadedkeys_.isEmpty(); }

protected:

			ObjectLoader(const DBKeySet&,
				     const SurfaceIODataSelection*);

    virtual void	addObject( Object* obj ) { emobjects_ += obj; }

    DBKeySet		dbkeys_;
    const SurfaceIODataSelection* sel_;
    DBKeySet		notloadedkeys_;
    RefObjectSet<Object> emobjects_;

    friend class	ObjectLoaderExec;

private:

    Executor*		fetchLoader(Object*) const;

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

    virtual bool	load(TaskRunner*);
    virtual Executor*	getLoader() const;

protected:

    void		addObject( Object* obj ) { emobjects_ += obj; }
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

     virtual bool	load(TaskRunner*);
     virtual Executor*	getLoader() const;

protected:

    void		addObject( Object* obj ) { emobjects_ += obj; }
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

     virtual bool	load(TaskRunner*);
     virtual Executor*	getLoader() const;

protected:

    void		addObject( Object* obj ) { emobjects_ += obj; }
};


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

    virtual uiRetVal	doStore(const IOObj&,TaskRunner*) const;
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

    virtual uiRetVal	doStore(const IOObj&,TaskRunner*) const;
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

    virtual uiRetVal	doStore(const IOObj&,TaskRunner*) const;
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

    virtual uiRetVal	doStore(const IOObj&,TaskRunner*) const;
};

} // namespace EM

