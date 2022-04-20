#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jun 2006
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "callback.h"
#include "uidialog.h"
#include "uistring.h"

class BufferStringSet;
class IOObj;
class uiParent;
class uiTextBrowser;
namespace Pick { class Set; class SetMgr; }


/*! \brief base class for management of a Pick::SetMgr */

mExpClass(uiIo) uiPickSetMgr : public CallBacker
{
mODTextTranslationClass(uiPickSetMgr)
public:
			uiPickSetMgr(uiParent*,Pick::SetMgr&);
			~uiPickSetMgr();

    bool		storeSets();	//!< Stores all changed sets
    bool		storeSet(const Pick::Set&);
    bool		storeSetAs(const Pick::Set&);
    void		mergeSets(MultiID&, const BufferStringSet* nms=0);
    bool		pickSetsStored() const;

    virtual bool	storeNewSet(const Pick::Set&) const;
    bool		storeNewSet(const Pick::Set&,bool noconf) const;
    void		keyPressedCB(CallBacker*);
    void		surveyChangeCB(CallBacker*);

protected:

    uiParent*		parent_;
    Pick::SetMgr&	setmgr_;

    virtual IOObj*	getSetIOObj(const Pick::Set&) const;
    virtual bool	doStore(const Pick::Set&,const IOObj&) const;
};



/*! \brief Dialog to show information on PickSet Manager */

mExpClass(uiIo) uiPickSetMgrInfoDlg : public uiDialog
{ mODTextTranslationClass(uiPickSetMgrInfoDlg);
public:
			uiPickSetMgrInfoDlg(uiParent*);
			~uiPickSetMgrInfoDlg();

    void		refresh(CallBacker*);

protected:
    uiTextBrowser*	browser_;
};
