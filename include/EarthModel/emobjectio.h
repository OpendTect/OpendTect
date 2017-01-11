#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		6-01-2017
________________________________________________________________________


-*/

#include "earthmodelmod.h"

#include "dbkey.h"
#include "emfaultstickset.h"
#include "saveable.h"


namespace EM
{

mExpClass(EarthModel) ObjectLoader
{
public:
		    
    virtual bool	load(TaskRunner*) = 0;
    virtual Executor*	getLoader() const = 0;
    ObjectSet<EMObject>	getLoadedEMObjects() const { return emobjects_; }

protected:

			ObjectLoader(const DBKey&);
			ObjectLoader(const DBKeySet&);

    DBKeySet		dbkeys_;
    ObjectSet<EMObject>	emobjects_;
};


mExpClass(EarthModel) FaultStickSetLoader : public ObjectLoader
{
public:
			FaultStickSetLoader(const DBKey&);
			FaultStickSetLoader(const DBKeySet&);

     virtual bool	load(TaskRunner*);
     virtual Executor*	getLoader() const;

     const DBKeySet&	tobeLodedKeys() const { return dbkeys_; }
      bool		allOK() const
			{ return dbkeys_.size() == loadedkeys_.size(); }
protected:

    void		addObject(EMObject* obj) { emobjects_ += obj; }
    DBKeySet		loadedkeys_;
    friend class	FSSLoaderExec;
};


mExpClass(EarthModel) ObjectSaver : public Saveable
{
public:

			ObjectSaver(const EMObject&);
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
			FaultStickSetSaver(const EMObject&);
			~FaultStickSetSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&) const;
};


} // namespace EM

