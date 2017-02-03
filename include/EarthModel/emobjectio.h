#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		6-01-2017
________________________________________________________________________


-*/

#include "earthmodelmod.h"

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

class EMObject;

mExpClass(EarthModel) ObjectLoader
{
public:
    mDefineFactory1ParamInClass(ObjectLoader,const DBKeySet&,factory)

    virtual uiString	userName()		= 0;
    virtual bool	load(TaskRunner*)	= 0;
    virtual Executor*	getLoader() const	= 0;

    ObjectSet<EMObject> getLoadedEMObjects() const { return emobjects_; }
    const DBKeySet&	tobeLodedKeys() const { return dbkeys_; }
    virtual bool	allOK() const
			{ return notloadedkeys_.isEmpty(); }
protected:
			ObjectLoader(const DBKeySet&);

    virtual void	addObject(EMObject* obj) { emobjects_ += obj; }

    DBKeySet		dbkeys_;
    DBKeySet		notloadedkeys_;
    ObjectSet<EMObject> emobjects_;

    friend class	ObjectLoaderExec;
};


mExpClass(EarthModel) FaultStickSetLoader : public ObjectLoader
{
public:

    mDefaultFactoryInstantiation1Param(ObjectLoader,
				       FaultStickSetLoader,const DBKeySet&,
				       EM::FaultStickSet::typeStr(),
				       uiStrings::sFaultStickSet(mPlural))

			FaultStickSetLoader(const DBKeySet&);

    uiString		userName() {return uiStrings::sFaultStickSet(mPlural);}

    virtual bool	load(TaskRunner*);
    virtual Executor*	getLoader() const;

protected:

    void		addObject(EMObject* obj) { emobjects_ += obj; }
};


mExpClass(EarthModel) Fault3DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation1Param(ObjectLoader,
				       Fault3DLoader,const DBKeySet&,
				       Fault3D::typeStr(),
				       uiStrings::sFault(mPlural))

			Fault3DLoader(const DBKeySet&);

    uiString		userName() { return uiStrings::sFault(mPlural); }

     virtual bool	load(TaskRunner*);
     virtual Executor*	getLoader() const;

protected:

    void		addObject(EMObject* obj) { emobjects_ += obj; }
};


mExpClass(EarthModel) Horizon3DLoader : public ObjectLoader
{
public:
      mDefaultFactoryInstantiation1Param(ObjectLoader,
				       Horizon3DLoader,const DBKeySet&,
				       Horizon3D::typeStr(),
				       uiStrings::sHorizon(mPlural))

			Horizon3DLoader(const DBKeySet&);

    uiString		userName() { return uiStrings::sHorizon(mPlural); }

     virtual bool	load(TaskRunner*);
     virtual Executor*	getLoader() const;

protected:

    void		addObject(EMObject* obj) { emobjects_ += obj; }
};


mExpClass(EarthModel) ObjectSaver : public Saveable
{
public:

    mDefineFactory1ParamInClass(ObjectSaver,const SharedObject&,factory)

			ObjectSaver(const SharedObject&);
			mDeclMonitorableAssignment(ObjectSaver);
			~ObjectSaver();

    ConstRefMan<EMObject> emObject() const;
    void		  setEMObject(const EMObject&);

    mDeclInstanceCreatedNotifierAccess(ObjectSaver);

protected:

    virtual uiRetVal	doStore(const IOObj&) const;
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

    virtual uiRetVal	doStore(const IOObj&) const;
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

    virtual uiRetVal	doStore(const IOObj&) const;
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

    virtual uiRetVal	doStore(const IOObj&) const;
};

} // namespace EM

