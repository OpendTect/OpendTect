#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "callback.h"
#include "integerid.h"
#include "sets.h"

class SoSelection;
class SoNode;
class SoPath;

namespace Threads { class Mutex; }

namespace visBase
{

class Scene;
class DataObject;
class DataManager;
class Selectable;

/*!\brief
SelectionManager handles DataObject that can be selected. If an object can be
selected, it has to register himself with regSelObject. At registration it has
to give two objects, first of all, he gives the object that outside users
will associate him with when they want to add their CB to detect his selection.

Secondly it has to give the DataObject that actually is selected.
*/

mExpClass(visBase) SelectionManager : public CallBacker
{
public:
				SelectionManager();
    virtual			~SelectionManager();

    void			setAllowMultiple(bool yn);
    bool			allowMultiple() const { return allowmultiple_; }

    void			select(VisID id,bool keepoldsel=false)
				{ select( id, keepoldsel, true ); }
    void			deSelect( VisID id )	{ deSelect( id, true); }
    void			deSelectAll()		{ deSelectAll( true ); }
    void			updateSel( VisID id )	{ updateSel( id, true);}

    const TypeSet<VisID>&	selected() const  { return selectedids_; }

    CNotifier<SelectionManager,VisID>	selnotifier;
    CNotifier<SelectionManager,VisID>	deselnotifier;
    CNotifier<SelectionManager,VisID>	updateselnotifier;

    				// for implementing pick-based reselection
    CNotifier<SelectionManager,VisID>	reselnotifier;

    void			fillPar(IOPar&,TypeSet<int>&) const {}
    void			usePar(const IOPar&) {}

protected:
    void			select(VisID,bool keep,bool lock);
    void			deSelect(VisID,bool lock);
    void			deSelectAll(bool lock);
    void			updateSel(VisID,bool lock);

    TypeSet<VisID>		selectedids_;
    bool			allowmultiple_;
    Threads::Mutex&		mutex_;
};

} // namespace visBase
