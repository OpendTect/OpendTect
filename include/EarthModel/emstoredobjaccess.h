#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2016
________________________________________________________________________


-*/

#include "emcommon.h"
#include "objectset.h"
#include "uistring.h"

namespace EM
{

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
		StoredObjAccess(const DBKey&);
    virtual	~StoredObjAccess();

    void	setLoadHint(const SurfaceIODataSelection&);
    bool	set(const DBKey&);

    bool	isReady(int iobj=-1) const;
    bool	isError(int iobj=-1) const;
    float	ratioDone(int iobj=-1) const;
    uiString	getError(int iobj=-1) const;

    bool	finishRead();	//!< may take a long time
				//!< will 'run' until reading is finished

    Object*	object(int iobj=0); //!< returns null until ready
    const Object* object(int iobj=0) const;

		// Interesting for multi-read only
    int		size() const		{ return data_.size(); }
    bool	isEmpty() const		{ return data_.isEmpty(); }
    bool	add(const DBKey&);
    void	dismiss(const DBKey&);

protected:

    ObjectSet<StoredObjAccessData>	data_;
    SurfaceIODataSelection*		surfiodsel_;
    StoredObjAccessData*		get(const DBKey&);

};



} // namespace EM
