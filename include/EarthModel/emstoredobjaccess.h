#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "objectset.h"
#include "uistring.h"
class Executor;

namespace EM
{
class EMObject;
class StoredObjAccessData;
class SurfaceIODataSelection;


/*!\brief Access to stored EM Objects.

 Will read object(s) in the background. Can load many objects, but works
 intuitively with a single object, too.

*/

mExpClass(EarthModel) StoredObjAccess
{ mODTextTranslationClass(StoredObjAccess)
public:

			StoredObjAccess();
			StoredObjAccess(const MultiID&);
    virtual		~StoredObjAccess();

    void		setLoadHint(const SurfaceIODataSelection&);
    bool		set(const MultiID&);

    bool		isReady(int iobj=-1) const;
    bool		isError(int iobj=-1) const;
    float		ratioDone(int iobj=-1) const;
    uiString		getError(int iobj=-1) const;

    bool		finishRead();	//!< may take a long time
    Executor*		reader();     //!< will 'run' until reading is finished

    EMObject*		object(int iobj=0); //!< returns null until ready
    const EMObject*	object(int iobj=0) const;

			// Interesting for multi-read only
    int			size() const		{ return data_.size(); }
    bool		isEmpty() const		{ return data_.isEmpty(); }
    bool		add(const MultiID&);
    void		dismiss(const MultiID&);

protected:

    ObjectSet<StoredObjAccessData>	data_;
    SurfaceIODataSelection*		surfiodsel_;
    StoredObjAccessData*		get(const MultiID&);

};

} // namespace EM
