/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiactiverunningproc.h"

#include "batchprogtracker.h"
#include "clientservicebase.h"

#include "uidialog.h"
#include "uilabel.h"


class uiActiveRunningProcDlg : public uiDialog
{ mODTextTranslationClass(uiActiveRunningProcDlg);
public:
    uiActiveRunningProcDlg( uiParent* p, const uiStringSet& descs )
	: uiDialog(p, uiDialog::Setup(tr("Active running processes"),
	    tr("The following processes are now currently running and "
		"will be closed when you close OpendTect"), mNoHelpKey))
    {
	const uiString txt( tr("Close all these processes along"
							" with OpendTect") );
	setOkText( txt );

	for ( int idx=0; idx<descs.size(); idx++)
	{
	    auto* label = new uiLabel( this, descs.get(idx) );
	    if (idx)
		label->attach( alignedBelow, labels_[idx-1] );

	    labels_ += label;
	}
    }

    ~uiActiveRunningProcDlg()
    {
	deepErase( labels_ );
    }


protected:
    ObjectSet<uiLabel>		labels_;

};


//ActiveProcPrompter
ActiveProcPrompter& ActiveProcPrompter::APP()
{
    mDefineStaticLocalObject(ActiveProcPrompter, res, );
    return res;
}


ActiveProcPrompter::ActiveProcPrompter()
{}

ActiveProcPrompter::~ActiveProcPrompter()
{
}


bool ActiveProcPrompter::doTrigger( uiParent* parent )
{
    uiStringSet procdetails;
    const TypeSet<Network::Service::ID>& servids = BPT().getServiceIDs();
    if ( servids.isEmpty() )
	return true;

    const uiString dispstr = servids.size() > 1 ?
	tr("Active Batch Processes : %1").arg(servids.size()) :
	tr("Active Batch Process");

    procdetails.add( dispstr );

    auto*  dlg_ = new uiActiveRunningProcDlg( parent, procdetails);
    dlg_->setModal( true );
    bool retval = dlg_->go();
    deleteAndNullPtr( dlg_ );

    return retval;
}
