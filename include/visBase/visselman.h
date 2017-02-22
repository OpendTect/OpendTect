#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
________________________________________________________________________


-*/


#include "visbasecommon.h"
#include "notify.h"

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

    void			select(int id,bool keepoldsel=false)
				{ select( id, keepoldsel, true ); }
    void			deSelect(int id)  { deSelect( id, true ); }
    void			deSelectAll()	  { deSelectAll( true ); }
    void			updateSel(int id) { updateSel( id, true ); }

    const TypeSet<int>&		selected() const  { return selectedids_; }

    CNotifier<SelectionManager,int>	selnotifier;
    CNotifier<SelectionManager,int>	deselnotifier;
    CNotifier<SelectionManager,int>	updateselnotifier;

				// for implementing pick-based reselection
    CNotifier<SelectionManager,int>	reselnotifier;

    void			fillPar(IOPar&,TypeSet<int>&) const {}
    void			usePar(const IOPar&) {}

protected:
    void			select(int id,bool keep,bool lock);
    void			deSelect(int id,bool lock);
    void			deSelectAll(bool lock);
    void			updateSel(int id,bool lock);

    TypeSet<int>		selectedids_;
    bool			allowmultiple_;
    Threads::Mutex&		mutex_;
};

} // namespace visBase
