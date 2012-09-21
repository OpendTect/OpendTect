#ifndef visselman_h
#define visselman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "callback.h"
#include "sets.h"

class IOPar;
class SoSelection;
class SoNode;
class SoPath;

namespace Threads { class Mutex; };

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

mClass(visBase) SelectionManager : public CallBacker
{
public:
				SelectionManager();
    virtual			~SelectionManager();

    void			setAllowMultiple(bool yn);
    bool			allowMultiple() const { return allowmultiple; }

    void			select( int id, bool keepoldsel = false )
				{ select( id, keepoldsel, true ); }
    void			deSelect( int id ) { deSelect( id, true ); }
    void			deSelectAll() { deSelectAll( true ); }

    const TypeSet<int>&		selected() const { return selectedids; }

    CNotifier<SelectionManager,int>	selnotifier;
    CNotifier<SelectionManager,int>	deselnotifier;

    				// for implementing pick-based reselection
    CNotifier<SelectionManager,int>	reselnotifier;

    void			fillPar( IOPar&, TypeSet<int>& ) const {};
    void			usePar( const IOPar& ) {};

protected:
    void			select( int id, bool keep, bool lock );
    void			deSelect( int id, bool lock );
    void			deSelectAll(bool lock);

    TypeSet<int>		selectedids;
    bool			allowmultiple;
    Threads::Mutex&		mutex;
};

};

#endif

