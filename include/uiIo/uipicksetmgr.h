#ifndef uipicksetmgr_h
#define uipicksetmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jun 2006
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "callback.h"
#include "uistring.h"

class BufferStringSet;
class IOObj;
class uiParent;
namespace Pick { class Set; class SetMgr; }


/*! \brief base class for management of a Pick::SetMgr */

mExpClass(uiIo) uiPickSetMgr : public CallBacker
{ mODTextTranslationClass(uiPickSetMgr);
public:
			uiPickSetMgr(uiParent*,Pick::SetMgr&);
			~uiPickSetMgr();

    bool		storeSets();	//!< Stores all changed sets
    bool		storeSet(const Pick::Set&);
    bool		storeSetAs(const Pick::Set&);
    void		mergeSets(MultiID&, const BufferStringSet* nms=0);
    bool		pickSetsStored() const;

    virtual bool	storeNewSet(Pick::Set*&,bool noconf=false) const;
    void		keyPressedCB(CallBacker*);

protected:

    uiParent*		parent_;
    Pick::SetMgr&	setmgr_;

    virtual IOObj*	getSetIOObj(const Pick::Set&) const;
    virtual bool	doStore(const Pick::Set&,const IOObj&) const;
};


#endif
