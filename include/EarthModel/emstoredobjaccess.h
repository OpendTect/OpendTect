#ifndef emstoredobjaccess_h
#define emstoredobjaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2016
 RCS:		$Id$
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


/*!\brief Access to stored EM Objects.

 Will read object(s) in the background. Can load many objects, but works
 intuitively with a single object, too.

*/

mExpClass(EarthModel) StoredObjAccess
{ mODTextTranslationClass(StoredObjAccess)
public:

		StoredObjAccess(const MultiID&);
		StoredObjAccess();
    virtual	~StoredObjAccess();

    bool	isReady(int iobj=-1) const;
    bool	isError(int iobj=-1) const;
    float	ratioDone(int iobj=-1) const;
    uiString	getError(int iobj=-1) const;

    bool	finishRead();	//!< may take a long time
    Executor*	reader();	//!< will 'run' until reading is finished

    const EMObject* object(int iobj=0) const;
    EMObject*	object(int iobj=0);

    Executor*	saver(int iobj=0);

		// Interesting for multi-read only
    int		size() const		{ return data_.size(); }
    bool	add(const MultiID&);
    void	dismiss(const MultiID&);

protected:

    ObjectSet<StoredObjAccessData>  data_;

};



} // namespace EM


#endif
