#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2006
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uisysadmmod.h"
#include "uidialog.h"
#include "uistring.h"
class uiODSysAdm;
class uiListBox;
class uiTextEdit;


uiODSysAdm& ODSysAdmMainWin();
//!< Top-level access

/*!
\brief OpendTect System Administration application top level object.
*/

mExpClass(uiSysAdm) uiODSysAdm : public uiDialog
{ mODTextTranslationClass(uiODSysAdm);
public:

			uiODSysAdm(uiParent*);
			~uiODSysAdm();
    struct TaskEntry
    {
				TaskEntry( const char* nm, const CallBack& cb,
					   const char* comm=0 )
				    : name_(nm)
				    , comment_(comm)
				    , cb_(cb)		{}

	BufferString		name_;
	BufferString		comment_;
	CallBack		cb_;
    };
    struct GroupEntry
    {
				GroupEntry( const char* nm )
				    : name_(nm)		{}

	BufferString		name_;
	ObjectSet<TaskEntry>	tasks_;
    };

    ObjectSet<GroupEntry>	groups_;

    GroupEntry*		getGroupEntry(const char*);
    TaskEntry*		getTaskEntry(GroupEntry*,const char*);

    const BufferString	swdir_;		//!< GetSoftwareDir()
    const BufferString	asdir_;		//!< GetApplSetupDir()
    const bool		haveas_;	//!< Does asdir_ exist?
    const bool		swwritable_;	//!< Is swdir_ writable?
    const bool		aswritable_;	//!< Is asdir_ writable?

protected:

    uiListBox*		grpfld;
    uiListBox*		taskfld;
    uiTextEdit*		commentfld;

    TaskEntry*		getCurTaskEntry();

    bool		acceptOK(CallBacker*);
    void		setInitial(CallBacker*);
    void		grpChg(CallBacker*);
    void		taskChg(CallBacker*);
    void		taskDClick(CallBacker*);

    void		doColorTabs(CallBacker*);
    void		doShortcuts(CallBacker*);
    void		doIconSets(CallBacker*);
    void		doBatchHosts(CallBacker*);
    void		doBatchProgs(CallBacker*);
    void		doInstLicFile(CallBacker*);
    void		doStartLic(CallBacker*);
    void		doAttribSets(CallBacker*);

};


