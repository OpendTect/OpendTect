#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "bufstring.h"
#include "callback.h"
#include "factory.h"
#include "multiid.h"
#include "emposid.h"
#include "emundo.h"
#include "enums.h"

class Undo;
class IOObj;
class IOObjContext;
class TaskRunner;
class Executor;
class uiEMPartServer;
class ZAxisTransform;

template <class T> class Selector;

namespace EM
{
class EMObject;
class SurfaceIOData;
class SurfaceIODataSelection;

enum class EMObjectType
{
    Hor2D	= 0,
    Hor3D	= 1,
    AnyHor	= 2,
    Flt3D	= 3,
    FltSS2D	= 4,
    FltSS3D	= 5,
    FltSS2D3D	= 6,
    FltSet	= 7,
    Body	= 8,
    Unknown	= 9
};

mDeclareNameSpaceEnumUtils(EarthModel,EMObjectType)


/*!
\brief Manages the loaded/half loaded EM objects in OpendTect.
*/

mExpClass(EarthModel) EMManager : public CallBacker
{
public:
			EMManager();
			~EMManager();

    inline int		nrLoadedObjects() const	{ return objects_.size(); }
    inline int		size() const		{ return nrLoadedObjects(); }
    EM::ObjectID	objectID(int idx) const;
    bool		objectExists(const EMObject*) const;

    EMObject*		loadIfNotFullyLoaded(const MultiID&,
				    TaskRunner* =nullptr);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */

    EMObject*		getObject(const ObjectID&);
    const EMObject*	getObject(const ObjectID&) const;
    EMObject*		getObject(const MultiID&);
    const EMObject*	getObject(const MultiID&) const;
    EMObject*		createTempObject(const char* type);

    BufferString	objectName(const MultiID&) const;
			/*!<\returns the name of the object */
    const char*		objectType(const MultiID&) const;
			/*!<\returns the type of the object */

    ObjectID		getObjectID(const MultiID&) const;
			/*!<\note that the relationship between storage id
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */
    MultiID		getMultiID(const ObjectID&) const;
			/*!<\note that the relationship between storage id
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */

    void		burstAlertToAll(bool yn);

    void		removeSelected(const ObjectID&,const Selector<Coord3>&,
				       TaskRunner*);
    bool		readDisplayPars(const MultiID&,IOPar&) const;
    bool		writeDisplayPars(const MultiID&,const IOPar&) const;
    bool		getSurfaceData(const MultiID&,SurfaceIOData&,
				       uiString& errmsg) const;

    Notifier<EMManager>	addRemove;

protected:

    mStruct(EarthModel) EMObjUndo
    {
	EMObjUndo(const EM::ObjectID& id)
	    : undo_(*new EMUndo()),id_(id) {}
	~EMObjUndo() { delete &undo_; }

	Undo&		undo_;
	EM::ObjectID	id_;
    };

    ObjectSet<EMObjUndo>	undolist_;

    ObjectSet<EMObject>		objects_;

    void		levelToBeRemoved(CallBacker*);
    static const char*	displayparameterstr();

    bool		readParsFromDisplayInfoFile(const MultiID&,
						    IOPar&) const;
    bool		readParsFromGeometryInfoFile(const MultiID&,
						     IOPar&) const;
    int			undoIndexOf(const EM::ObjectID&);

public:

    // Don't use unless you know what you are doing

    void		setEmpty();

    Executor*		objectLoader(const MultiID&,
				    const SurfaceIODataSelection* =nullptr);
    Executor*		objectLoader(const TypeSet<MultiID>&,
				    const SurfaceIODataSelection* =nullptr,
				    TypeSet<MultiID>* idstobeloaded =nullptr);
			/*!< idstobeloaded are the ids for which the objects
			     will be actually loaded */

    EM::ObjectID	createObject(const char* type,const char* name);
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */

			/*Interface from EMObject to report themselves */
    void		addObject(EMObject*);
    void		removeObject(const EMObject*);

    mDeprecatedDef
    Undo&		undo();
    mDeprecatedDef
    const Undo&		undo() const;

    void		eraseUndoList();
    Undo&		undo(const EM::ObjectID&);

private:
    Undo&			undo_;
				/*don't use it, only for keep ABI */
};


mDefineFactory1Param( EarthModel, EMObject, EMManager&, EMOF );

mGlobal(EarthModel) EMManager& EMM();
mGlobal(EarthModel) bool canOverwrite(const MultiID&);
mGlobal(EarthModel) bool isFaultStickSet(EM::EMObjectType);

} // namespace EM
