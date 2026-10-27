#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "integerid.h"
#include "typeset.h"

namespace Threads { class Mutex; }

namespace dGBVis
{

class DataManager;
class DataObject;

/*!\brief Which dGBVis objects are selected.

  Counterpart of visBase::SelectionManager. DataManager owns one and
  exposes it through selMan().
*/

mExpClass(vsgBase) SelectionManager
{
public:
				SelectionManager();
				~SelectionManager();

    void			select(const VisID& id)
				{ select(id,true); }
    void			deSelect(const VisID& id)
				{ deSelect(id,true); }
    void			deSelectAll()		{ deSelectAll(true); }

    const TypeSet<VisID>&	selected() const	{ return selectedids_; }

protected:
    void			select(const VisID&,bool lock);
    void			deSelect(const VisID&,bool lock);
    void			deSelectAll(bool lock);

    TypeSet<VisID>		selectedids_;
    Threads::Mutex&		mutex_;

private:
    friend class		DataManager;
    void			drop(const VisID&);
				/*!<Remove an id without telling the object.
				    Used when the object is being deleted. */
};

} // namespace dGBVis
