#ifndef uipicksetmgr_h
#define uipicksetmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jun 2006
 RCS:           $Id: uipicksetmgr.h,v 1.4 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"

class IOObj;
class MultiID;
class uiParent;
namespace Pick { class Set; class SetMgr; };


/*! \brief base class for management of a Pick::SetMgr */

mClass uiPickSetMgr : public CallBacker
{
public:
			uiPickSetMgr(Pick::SetMgr&);

    bool		storeSets();	//!< Stores all changed sets
    bool		storeSet(const Pick::Set&);
    bool		storeSetAs(const Pick::Set&);
    void		mergeSets(MultiID&);
    bool		pickSetsStored() const;

    virtual uiParent*	parent()		= 0;

protected:

    Pick::SetMgr&	setmgr_;

    virtual bool	storeNewSet(Pick::Set*&) const;
    virtual IOObj*	getSetIOObj(const Pick::Set&) const;
    virtual bool	doStore(const Pick::Set&,const IOObj&) const;

};


#endif
