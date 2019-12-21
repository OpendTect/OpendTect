/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2016
________________________________________________________________________

-*/

#include "uiautosaverestore.h"
#include "uiautosavesettings.h"
#include "uiodmain.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uichecklist.h"
#include "uimsg.h"
#include "dbman.h"
#include "iostrm.h"
#include "keystrs.h"
#include "transl.h"
#include "autosaver.h"
#include "ioobjctxt.h"
#include "od_helpids.h"
#include "separstr.h"
#include "genc.h"
#include "oddirs.h"


/*\brief Allows user to create proper objects from auto-saved copies */

class AutoSaved2RealObjectRestorer : public CallBacker
{ mODTextTranslationClass(AutoSaved2RealObjectRestorer)
public:

AutoSaved2RealObjectRestorer()
{
    doWork( 0 );
    mAttachCB( DBM().surveyChanged, AutoSaved2RealObjectRestorer::doWork );
}

void doWork( CallBacker* )
{
    if ( uiAutoSaverSettingsGroup::autoAskRestore() )
	uiAutoSave2RealObjDlg::run4All( GetLocalHostName(), GetUserNm() );
}

}; // class AutoSaved2RealObjectRestorer

static AutoSaved2RealObjectRestorer* autosaved_2_realobj_restorer = 0;

void startAutoSaved2RealObjectRestorer()
{
    Monitorable::AccessLocker::enableLocking( true );
    autosaved_2_realobj_restorer = new AutoSaved2RealObjectRestorer;
}



uiAutoSave2RealObjDlg::uiAutoSave2RealObjDlg( uiParent* p, IOObj& ioobj,
						int curidx, int totalnr )
    : uiDialog(p,uiDialog::Setup(
	    tr("Restore Auto-Saved Object (%1/%2)").arg(curidx+1).arg(totalnr),
	    mNoDlgTitle,mTODOHelpKey))
    , ioobj_(ioobj)
{
    const BufferString objtyp( ioobj_.group() );
    BufferString orgnm, newnm;
    FixedString crfrom = ioobj_.pars().find( sKey::CrFrom() );
    if ( !crfrom.isEmpty() )
    {
	FileMultiString fms( crfrom );
	BufferString nm( fms[1] );
	if ( !nm.isEmpty() )
	{
	    orgnm.set( nm );
	    newnm.set( orgnm ).add( " [recovered]" );
	}
    }
    if ( !orgnm.isEmpty() )
	setTitleText( tr("Found auto-saved object for '%1'").arg(orgnm) );
    else
    {
	newnm.set( "Recovered " ).add( objtyp );
	orgnm.set( "<No Name>" );
    }

    FileMultiString fms( ioobj.pars().find( sKey::CrInfo() ) );
    const BufferString hostnm( fms[0] );
    const BufferString usrnm( ioobj.pars().find(sKey::CrBy()) );
    const BufferString savedatetime( ioobj.pars().find(sKey::CrAt()) );
    const BufferString dtectusr( fms[2] );
    BufferString txt;
    txt.set( "Name: " ).add( orgnm )
       .add( "\nType: " ).add( objtyp )
       .add( "\nHost: " ).add( hostnm )
       .add( "\nUser: " ).add( usrnm );
    if ( !dtectusr.isEmpty() )
	txt.add( " [" ).add( dtectusr ).add( "]" );
    txt.add( "\nWhen: " ).add( savedatetime );

    infofld_ = new uiTextEdit( this, "Object Info", true );
    infofld_->setPrefHeightInChar( 5 );
    infofld_->setStretch( 2, 2 );
    infofld_->setText( txt );

    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItem( tr("[Restore] Save this %1 under a new name")
			.arg(objtyp) );
    choicefld_->addItem( tr("[Delete] Delete this version of %1").arg(orgnm) );
    if ( curidx<totalnr-1 )
	choicefld_->addItem( tr("[Delete-All] Delete all auto-saved objects.")
			    );
    choicefld_->addItem( tr("[Cancel] Postpone (upto 1 week after creation).")
			    );
    choicefld_->setChecked( 0, true );
    choicefld_->changed.notify( mCB(this,uiAutoSave2RealObjDlg,choiceSel) );
    choicefld_->attach( alignedBelow, infofld_ );

    newnmfld_ = new uiGenInput( this, tr("Save under name"),
				StringInpSpec(newnm) );
    newnmfld_->attach( alignedBelow, choicefld_ );
    newnmfld_->setElemSzPol( uiObject::WideMax );
}


void uiAutoSave2RealObjDlg::choiceSel( CallBacker* )
{
    newnmfld_->display( choicefld_->firstChecked() == 0 );
}


bool uiAutoSave2RealObjDlg::acceptOK()
{
    return true;
}


bool uiAutoSave2RealObjDlg::isCancel() const
{
    return choicefld_->firstChecked() == choicefld_->size() - 1;
}


int uiAutoSave2RealObjDlg::run4All( const char* hnm, const char* unm )
{
    ObjectSet<IOObj> ioobjs;
    IOObjSelConstraints cnstrts;
    cnstrts.allownonuserselectable_ = true;
    cnstrts.require_.set( "Auto-saved", sKey::Yes() );
    DBM().findTempObjs( ioobjs, &cnstrts );
    if ( ioobjs.isEmpty() )
	return 0;

    const BufferString reqhostnm( hnm );
    const BufferString requsrnm( unm );

    const BufferString localhostnm( GetLocalHostName() );
    bool delall = false;
    ObjectSet<IOObj> todoioobjs;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj& ioobj = *ioobjs[idx];

	FileMultiString fms( ioobj.pars().find( sKey::CrInfo() ) );
	const int pid( fms.getIValue(1) );
	const BufferString hostnm( fms[0] );
	const BufferString procnm( GetProcessNameForPID(pid) );
	const BufferString odprocnm( GetProcessNameForPID(GetPID()) );
	if ( hostnm == localhostnm && procnm == odprocnm )
	    continue; // another instance of od_main is running and made this
	else if ( !reqhostnm.isEmpty() && reqhostnm != hostnm )
	    continue;

	fms.set( ioobj.pars().find( sKey::CrBy() ) );
	const BufferString usrnm( fms[0] );
	if ( !requsrnm.isEmpty() && usrnm != requsrnm )
	    continue;

	todoioobjs += &ioobj;
    }

    for ( int idx=0; idx<todoioobjs.size(); idx++ )
    {
	IOObj& ioobj = *todoioobjs[idx];
	bool delthisone = false;
	if ( !delall )
	{
	    uiAutoSave2RealObjDlg dlg( ODMainWin(), ioobj, idx,
					todoioobjs.size() );
	    if ( !dlg.go() || dlg.isCancel() )
		break;

	    const int choice = dlg.choicefld_->firstChecked();
	    if ( choice == 0 )
		doRestore( ioobj, dlg.newnmfld_->text() );
	    else if ( choice == 2 ) // cannot be cancel, checked earlier
		delall = true;
	    else
		delthisone = true;
	}

	if ( delthisone || delall )
	{
	    ioobj.implRemove();
	    ioobj.removeFromDB();
	}
    }

    deepErase( ioobjs );
    return todoioobjs.size();
}


void uiAutoSave2RealObjDlg::doRestore( IOObj& ioobj, const char* newnm )
{
    mDynamicCastGet(IOStream*,iostrm,&ioobj);
    if ( !iostrm )
    {
	BufferString msg( "Object ", ioobj.name() );
	msg.add( "is not IOStream; conntype=" ).add( ioobj.connType() );
	gUiMsg().error( mINTERNAL(msg) );
	return;
    }

    while ( !OD::AUTOSAVE().restore( *iostrm, newnm ) )
    {
	uiString msg( tr("Failed to commit the restore process."
	"\nYou may want to check your storage (write permissions, disk full)."
	"\nIf retry fails, %1" ).arg( uiStrings::phrPlsContactSupport(false)));
	if ( !gUiMsg().askGoOn(msg,tr("Retry restore"),tr("Cancel restore")) )
	    break;
    }
}
