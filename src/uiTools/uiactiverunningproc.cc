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
	: uiDialog(p,uiDialog::Setup(tr("Active Batch Processes"),
	  tr("The Batch processes will keep running in the background,\n"
	     "even after OpendTect is closed"),mNoHelpKey))
    {
	setCtrlStyle( CtrlStyle::CloseOnly );

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
    TypeSet<Network::Service::ID> servids;
    if ( !BPT().getLiveServiceIDs(servids) )
	return true;

    const uiString dispstr =
	    tr("Number of Active Batch Processes : %1").arg( servids.size() );

    procdetails.add( dispstr );

    auto*  dlg_ = new uiActiveRunningProcDlg( parent, procdetails );
    dlg_->setModal( true );
    dlg_->go();
    deleteAndNullPtr( dlg_ );

    return true;
}
