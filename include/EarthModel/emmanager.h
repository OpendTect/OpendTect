#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emundo.h"
#include "factory.h"
#include "notify.h"
#include "ptrman.h"
#include "ranges.h"
#include "dbkey.h"
#include "taskrunner.h"
#include "saveablemanager.h"

class Undo;
class IOObj;
class IOObjContext;
class Executor;
class uiEMPartServer;

template <class T> class Selector;

namespace EM
{

class Manager;
class Object;
class ObjectLoader;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!\brief Your access point to all stored EM objects. */
mGlobal(EarthModel) Manager& MGR();


/*!\brief Manages the loaded/half loaded EM objects in OpendTect.  */

mExpClass(EarthModel) ObjectManager : public SaveableManager
{
public:

			ObjectManager(const IOObjContext&);
			~ObjectManager();

    inline int		nrLoadedObjects() const	{ return savers_.size(); }
    inline int		size() const		{ return nrLoadedObjects(); }
    ObjID		objID(int idx) const;
    bool		objectExists(const Object*) const;

    Object*		loadIfNotFullyLoaded(const ObjID&,
					     const TaskRunnerProvider&);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */

    RefObjectSet<Object> loadObjects(const ObjIDSet&,const TaskRunnerProvider&,
				     const SurfaceIODataSelection* =0);

    virtual ConstRefMan<Object>	fetch(const ObjID&,const TaskRunnerProvider&,
				      const SurfaceIODataSelection* =0,
				      bool forcereload=false) const;
    virtual RefMan<Object>	fetchForEdit(const ObjID&,
					     const TaskRunnerProvider&,
					     const SurfaceIODataSelection* =0,
					     bool forcereload=false);

    uiRetVal		store(const Object&,const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
    uiRetVal		store(const Object&,const ObjID&,
			      const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;

    virtual Object*	getObject(const ObjID&);

    Object*		createTempObject(const char* type);

    bool		is2D(const ObjID&) const;
    BufferString	objectName(const ObjID&) const;
			/*!<\returns the name of the object */
    BufferString	objectType(const ObjID&) const;
			/*!<\returns the type of the object */

    void		burstAlertToAll(bool yn);

    void		removeSelected(const ObjID&,const Selector<Coord3>&,
				       const TaskRunnerProvider&);
    static bool		readDisplayPars(const ObjID&,IOPar&);
    static bool		writeDisplayPars(const ObjID&,const IOPar&);
    bool		getSurfaceData(const ObjID&,SurfaceIOData&,
				       uiString& errmsg) const;

    Notifier<ObjectManager>	addRemove;

protected:

    Object*		gtObject(const ObjID&);
    virtual Saveable*	getSaver(const SharedObject&) const;
    virtual ChangeRecorder* getChangeRecorder(const SharedObject&) const
			{ return 0; }


    mStruct(EarthModel) ObjUndo
    {
			ObjUndo( const ObjID& id )
			    : id_(id)		{}
	Undo		undo_;
	ObjID		id_;
    };
    ObjectSet<ObjUndo>	undolist_;

    void		levelSetChgCB(CallBacker*);
    static const char*	displayparameterstr();

    static bool		readParsFromDisplayInfoFile(const ObjID&,IOPar&);
    static bool		readParsFromGeometryInfoFile(const ObjID&,IOPar&);
    int			undoIndexOf(const ObjID& id);

public:

    // Don't use unless you know what you are doing

    void		setEmpty();

    ObjectLoader*	objectLoader(const ObjID&,
				     const SurfaceIODataSelection* =0);
    ObjectLoader*	objectLoader(const ObjIDSet&,
				     const SurfaceIODataSelection* =0,
				     ObjIDSet* includedids=0);
			    /*!< includedids are the ids of the objects
				 that will be loaded */

    Object*		createObject(const char* type,const char* nm);
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */

    virtual void	addObject(Object*);

    void		eraseUndoList();
    Undo&		undo(const ObjID&);

};


mDefineFactory1Param( EarthModel, Object, ObjectManager&, EMOF );

mGlobal(EarthModel) bool canOverwrite(const ObjectManager::ObjID&);

mGlobal(EarthModel) ObjectManager& Hor3DMan();
mGlobal(EarthModel) ObjectManager& Hor2DMan();
mGlobal(EarthModel) ObjectManager& FSSMan();
mGlobal(EarthModel) ObjectManager& Flt3DMan();
mGlobal(EarthModel) ObjectManager& BodyMan();
mGlobal(EarthModel) ObjectManager& getMgr(const ObjectManager::ObjID&);
mGlobal(EarthModel) ObjectManager& getMgr(const char* trgrp);

/*!
\brief Manages all types of EM objects in OpendTect.
*/

mExpClass(EarthModel) Manager : public ObjectManager
{
public:

    ConstRefMan<Object> fetch(const ObjID& id,const TaskRunnerProvider& tp,
			      const SurfaceIODataSelection* sel=0,
			      bool forcereload=false) const
			{ return getMgr(id).fetch(id,tp,sel,forcereload); }
    RefMan<Object>	fetchForEdit(const ObjID& id,
				     const TaskRunnerProvider& tp,
				     const SurfaceIODataSelection* sel=0,
				     bool forcereload=false)
			{return getMgr(id).fetchForEdit(id,tp,sel,forcereload);}

    Object*		getObject(const ObjID& id)
			{ return getMgr(id).getObject( id ); }

    Object*		createTempObject(const char* type);

    void		removeSelected(const ObjID& id,
				       const Selector<Coord3>& sel,
				       const TaskRunnerProvider& trprov)
			{ return getMgr(id).removeSelected( id, sel, trprov ); }
    bool		readDisplayPars(const ObjID& id,IOPar& pars) const
			{ return getMgr(id).readDisplayPars( id, pars ); }
    bool		writeDisplayPars(const ObjID& id,
					 const IOPar& pars) const
			{ return getMgr(id).writeDisplayPars( id, pars ); }
    bool		getSurfaceData(const ObjID& id,SurfaceIOData& sd,
				       uiString& errmsg) const
			{ return getMgr(id).getSurfaceData( id, sd, errmsg ); }


    Object*		loadIfNotFullyLoaded(const ObjID& id,
					     const TaskRunnerProvider& tp)
			{ return getMgr(id).loadIfNotFullyLoaded( id, tp ); }

    Object*		createObject(const char* type,const char* nm)
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */
			{ return getMgr(type).createObject( type, nm ); }

    void		addObject(Object*);

    Undo&		getUndo(const ObjID& id)
			{ return getMgr(id).undo(id); }
    const Undo&		getUndo(const ObjID& id) const
			{ return getMgr(id).undo(id); }

protected:

			Manager();
    friend Manager&	MGR();

};

mDeprecated inline Manager& EMM() { return MGR(); }



mExpClass(EarthModel) FaultSetManager : public ObjectManager
{
public:

				    FaultSetManager();

    virtual ConstRefMan<Object>	    fetch(const ObjID&,
					  const TaskRunnerProvider&,
					  const SurfaceIODataSelection* =0,
					  bool forcereload=false) const;
};

mGlobal(EarthModel) FaultSetManager& FltSetMan();


} // namespace EM
