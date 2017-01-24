#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "factory.h"
#include "notify.h"
#include "ptrman.h"
#include "ranges.h"
#include "saveablemanager.h"

class Undo;
class IOObj;
class IOObjContext;
class TaskRunner;
class Executor;
class uiEMPartServer;

template <class T> class Selector;

namespace EM
{
class EMObject;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!
\brief Manages the loaded/half loaded EM objects in OpendTect.
*/

mExpClass(EarthModel) EMManager : public SaveableManager
{
public:

			EMManager(const IOObjContext&);
			~EMManager();

    inline int		nrLoadedObjects() const	{ return savers_.size(); }
    inline int		size() const		{ return nrLoadedObjects(); }
    DBKey		objID(int idx) const;
    bool		objectExists(const EMObject*) const;

    EMObject*		loadIfNotFullyLoaded(const DBKey&,TaskRunner* =0);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */

    ConstRefMan<EMObject> fetch(const DBKey&,TaskRunner* trunner=0,
			        bool forcereload=false) const;
    RefMan<EMObject>	fetchForEdit(const DBKey&,TaskRunner* trunner=0,
				     bool forcereload=false);

    uiRetVal		store(const EMObject&,const IOPar* ioobjpars=0) const;
    uiRetVal		store(const EMObject&,const ObjID&,
				const IOPar* ioobjpars=0) const;

    EMObject*		getObject(const DBKey&);

    EMObject*		createTempObject(const char* type);

    BufferString	objectName(const DBKey&) const;
			/*!<\returns the name of the object */
    const char*		objectType(const DBKey&) const;
			/*!<\returns the type of the object */

    void		burstAlertToAll(bool yn);

    void		removeSelected(const DBKey&,const Selector<Coord3>&,
				       TaskRunner*);
    bool		readDisplayPars(const DBKey&,IOPar&) const;
    bool		writeDisplayPars(const DBKey&,const IOPar&) const;
    bool		getSurfaceData(const DBKey&,SurfaceIOData&,
				       uiString& errmsg) const;

    Notifier<EMManager>	addRemove;

protected:

    EMObject*		gtObject(const DBKey&);
    virtual Saveable*	getSaver(const SharedObject&) const;
    virtual ChangeRecorder* getChangeRecorder(const SharedObject&) const
			{ return 0; }


    Undo&			undo_;

    void		levelSetChgCB(CallBacker*);
    static const char*	displayparameterstr();

    bool		readParsFromDisplayInfoFile(const DBKey&,
						    IOPar&)const;
    bool		readParsFromGeometryInfoFile(const DBKey&,
						     IOPar&)const;

public:

    // Don't use unless you know what you are doing

    void		setEmpty();

    Executor*		objectLoader(const DBKey&,
				     const SurfaceIODataSelection* =0);
    Executor*		objectLoader(const DBKeySet&,
				     const SurfaceIODataSelection* =0,
				     DBKeySet* idstobeloaded =0);
		/*!< idstobeloaded are the ids for which the objects
			     will be actually loaded */

    EMObject*		createObject(const char* type,const char* nm);
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */

			/*Interface from EMObject to report themselves */
    void		addObject(EMObject*);

    Undo&		undo();
    const Undo&		undo() const;

};


mDefineFactory1Param( EarthModel, EMObject, EMManager&, EMOF );

mGlobal(EarthModel) bool canOverwrite(const DBKey&);

mGlobal(EarthModel) EMManager& Hor3DMan();
mGlobal(EarthModel) EMManager& Hor2DMan();
mGlobal(EarthModel) EMManager& FSSMan();
mGlobal(EarthModel) EMManager& Flt3DMan();
mGlobal(EarthModel) EMManager& BodyMan();
mGlobal(EarthModel) EMManager& getMgr(const DBKey& objkey);
mGlobal(EarthModel) EMManager& getMgr(const char* trgrp);

/*!
\brief Manages all types of EM objects in OpendTect.
*/

mExpClass(EarthModel) GenEMManager : public EMManager
{
public:

			GenEMManager(const IOObjContext&);

    ConstRefMan<EMObject> fetch(const DBKey& id,TaskRunner* trunner=0,
			        bool forcereload=false) const
			{ return getMgr(id).fetch(id,trunner,forcereload); }
    RefMan<EMObject>	fetchForEdit(const DBKey& id,TaskRunner* trunner=0,
				     bool forcereload=false)
			{ return getMgr(id).fetchForEdit(id,trunner,
							 forcereload); }

    EMObject*		getObjectForEdit(const DBKey& id)
			{ return getMgr(id).getObject( id ); }

    EMObject*		createTempObject(const char* type);

    void		removeSelected(const DBKey& id,
				       const Selector<Coord3>& sel,
				       TaskRunner* trun)
			{ return getMgr(id).removeSelected( id, sel, trun ); }
    bool		readDisplayPars(const DBKey& id,IOPar& pars) const
			{ return getMgr(id).readDisplayPars( id, pars ); }
    bool		writeDisplayPars(const DBKey& id,
					 const IOPar& pars) const
			{ return getMgr(id).writeDisplayPars( id, pars ); }
    bool		getSurfaceData(const DBKey& id,SurfaceIOData& sd,
				       uiString& errmsg) const
			{ return getMgr(id).getSurfaceData( id, sd, errmsg ); }


    EMObject*		loadIfNotFullyLoaded(const DBKey& id,TaskRunner* t=0)
			{ return getMgr(id).loadIfNotFullyLoaded( id, t ); }
    Executor*		objectLoader(const DBKey& id,
				     const SurfaceIODataSelection* sd=0)
			{ return getMgr(id).objectLoader( id, sd ); }

    EMObject*		createObject(const char* type,const char* nm)
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */
			{ return getMgr(type).createObject( type, nm ); }

    Undo&		getUndo(const DBKey& id)
			{ return getMgr(id).undo(); }
    const Undo&		getUndo(const DBKey& id) const
			{ return getMgr(id).undo(); }

};

mGlobal(EarthModel) GenEMManager& EMM();

} // namespace EM
