/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2016
________________________________________________________________________

-*/

#include "uiodmain.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uichecklist.h"
#include "uiodmain.h"
#include "uimsg.h"
#include "ioman.h"
#include "iostrm.h"
#include "transl.h"
#include "autosaver.h"
#include "ctxtioobj.h"
#include "od_helpids.h"
#include "separstr.h"


class uiAutoSave2RealObjDlg : public uiDialog
{ mODTextTranslationClass(uiAutoSave2RealObjDlg)
public:

uiAutoSave2RealObjDlg( uiParent* p, IOObj& ioobj, bool havemore )
    : uiDialog(p,uiDialog::Setup(tr("Restore Auto-Saved Object"),mNoDlgTitle,
		    mTODOHelpKey))
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
    const BufferString osusr( ioobj.pars().find(sKey::CrBy()) );
    const BufferString savedatetime( ioobj.pars().find(sKey::CrAt()) );
    const BufferString dtectusr( fms[2] );
    BufferString txt;
    txt.set( "Name: " ).add( orgnm )
       .add( "\nType: " ).add( objtyp )
       .add( "\nHost: " ).add( hostnm )
       .add( "\nUser: " ).add( osusr );
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
    if ( havemore )
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


void choiceSel( CallBacker* )
{
    newnmfld_->display( choicefld_->firstChecked() == 0 );
}

bool acceptOK( CallBacker* )
{
    return true;
}

bool isCancel() const
{
    return choicefld_->firstChecked() == choicefld_->size() - 1;
}

    IOObj&	ioobj_;

    uiTextEdit*	infofld_;
    uiCheckList* choicefld_;
    uiGenInput*	newnmfld_;

}; // end class uiAutoSave2RealObjDlg


/*\brief Allows user to create proper objects from auto-saved copies */

class AutoSaved2RealObjectRestorer : public CallBacker
{ mODTextTranslationClass(AutoSaved2RealObjectRestorer)
public:

AutoSaved2RealObjectRestorer()
{
    doWork( 0 );
    mAttachCB( IOM().surveyChanged, AutoSaved2RealObjectRestorer::doWork );
}

void doWork( CallBacker* )
{
    ObjectSet<IOObj> ioobjs;
    IOObjSelConstraints cnstrts;
    cnstrts.allownonuserselectable_ = true;
    cnstrts.require_.set( "Auto-saved", sKey::Yes() );
    IOM().findTempObjs( ioobjs, &cnstrts );
    if ( ioobjs.isEmpty() )
	return;

    bool delall = false;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj& ioobj = *ioobjs[idx];
	// FileMultiString fms( ioobj.pars().find( sKey::CrInfo() ) );
	// const BufferString hostnm( fms[0] );
	// const int pid( fms.getIValue(1) );
	//TODO check pid (should not be running) and hostnm (should be same)

	bool delthisone = false;
	if ( !delall )
	{
	    uiAutoSave2RealObjDlg dlg( ODMainWin(), ioobj, idx<ioobjs.size()-1);
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
	    IOM().permRemove( ioobj.key() );
	}
    }

    deepErase( ioobjs );
}

void doRestore( IOObj& ioobj, const char* newnm )
{
    mDynamicCastGet(IOStream*,iostrm,&ioobj);
    if ( !iostrm )
	uiMSG().error( tr("Internal Error: Object has wrong type") );
    else if ( !OD::AUTOSAVE().restore( *iostrm, newnm ) )
    {
	uiString errmsg( tr("Failed to commit the restore process."
	"\nYou may want to check your storage (write permissions, disk full)."
	"\nOtherwise you may still be able to restore your work."
	"\nPlease contact OpendTect support at support@dgbes.com." ) );
	uiMSG().error( errmsg );
    }
}

}; // class AutoSaved2RealObjectRestorer

static AutoSaved2RealObjectRestorer* autosaved_2_realobj_restorer = 0;

void startAutoSaved2RealObjectRestorer()
{
    autosaved_2_realobj_restorer = new AutoSaved2RealObjectRestorer;
}
