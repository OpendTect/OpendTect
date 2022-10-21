/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobjmanip.h"

#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "transl.h"

#include "uibuttongroup.h"
#include "uifiledlg.h"
#include "uigeninputdlg.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "uitoolbutton.h"


// uiManipButGrp::ButData
uiManipButGrp::ButData::ButData( uiToolButton* b, const char* p,
				 const uiString& t )
	: but(b)
	, pmnm(p)
	, tt(t)
{
}


uiManipButGrp::ButData::~ButData()
{}



// uiManipButGrp
uiManipButGrp::uiManipButGrp( uiParent* p )
    : uiButtonGroup(p,"ManipButtons",OD::Vertical)
{
    altbutdata.allowNull();
}


uiManipButGrp::~uiManipButGrp()
{
    deepErase(butdata);
    deepErase(altbutdata);
}


uiToolButton* uiManipButGrp::addButton( Type tp, const uiString& tooltip,
					const CallBack& cb )
{
    const char* pm = 0;
    switch ( tp )
    {
	case FileLocation:	pm = "filelocation";	break;
	case Rename:		pm = "renameobj";	break;
	case Remove:		pm = "trashcan";	break;
	case ReadOnly:		pm = "readonly";	break;
	default:		pm = "home";
				pErrMsg("Unknown toolbut typ");
    }

    return addButton( pm, tooltip, cb );
}


uiToolButton* uiManipButGrp::addButton( const char* pmnm,
					const uiString& tooltip,
					const CallBack& cb )
{
    uiToolButton* button = new uiToolButton( this, pmnm, tooltip, cb );
    butdata += new ButData( button, pmnm, tooltip );
    altbutdata += 0;
    return button;
}


void uiManipButGrp::setAlternative( uiToolButton* button, const char* pm,
				    const uiString& tt )
{
    for ( int idx=0; idx<butdata.size(); idx++ )
    {
	if ( butdata[idx]->but == button )
	{
	    uiManipButGrp::ButData* bd = altbutdata[idx];
	    if ( !bd )
		altbutdata.replace( idx,
				    new uiManipButGrp::ButData(button,pm,tt) );
	    else
		{ bd->but = button; bd->pmnm = pm; bd->tt = tt; }
	}
    }
}


void uiManipButGrp::useAlternative( uiToolButton* button, bool yn )
{
    for ( int idx=0; idx<butdata.size(); idx++ )
    {
	uiManipButGrp::ButData* normbd = butdata[idx];
	if ( normbd->but == button )
	{
	    uiManipButGrp::ButData* altbd = altbutdata[idx];
	    if ( yn && !altbd ) return;
	    uiManipButGrp::ButData& bd = yn ? *altbd : *normbd;
	    button->setIcon( bd.pmnm );
	    button->setToolTip( bd.tt );
	    break;
	}
    }
}


uiIOObjManipGroup::uiIOObjManipGroup( uiIOObjManipGroupSubj& s, bool withreloc,
				      bool withremove )
	: uiManipButGrp(s.obj_->parent())
	, subj_(s)
	, locbut(0)
	, rembut(0)
{
    subj_.grp_ = this;

    const CallBack cb( mCB(this,uiIOObjManipGroup,tbPush) );
    if ( withreloc )
	locbut = addButton( FileLocation, tr("Change location on disk"), cb );
    renbut = addButton( Rename, uiStrings::phrRename(tr("this object")), cb );
    robut = addButton( ReadOnly, tr("Toggle Read only : locked"), cb );
    setAlternative( robut, "unlock", tr("Toggle Read only : editable") );
    if ( withremove )
	rembut = addButton( Remove, uiStrings::phrJoinStrings(
				    uiStrings::sDelete(), tr("Selected")), cb );
    attach( rightOf, subj_.obj_ );
}


uiIOObjManipGroup::~uiIOObjManipGroup()
{
}



void uiIOObjManipGroup::triggerButton( uiManipButGrp::Type tp )
{
    if ( tp == FileLocation && locbut )	locbut->click();
    else if ( tp == Rename )		renbut->click();
    else if ( tp == ReadOnly )		robut->click();
    else if ( tp == Remove && rembut )	rembut->click();
}


void uiIOObjManipGroup::selChg()
{
    const MultiID curid = subj_.currentID();
    IOObj* curioobj = IOM().get( curid );
    if ( !curioobj )
    {
	renbut->setSensitive( false );
	robut->setSensitive( false );
	if ( locbut ) locbut->setSensitive( false );
	if ( rembut ) rembut->setSensitive( false );
	return;
    }

    TypeSet<MultiID> chosenids; subj_.getChosenIDs( chosenids );
    BufferStringSet chosennames; subj_.getChosenNames( chosennames );
    if ( chosenids.isEmpty() )
	return;

    IOObj* firstchosenioobj = IOM().get( chosenids[0] );

    uiString tt;
#define mSetTBStateAndTT4Cur(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt = toUiString("%1 '%2'").arg(oper).arg(curioobj->uiName()); \
    tb->setToolTip( tt )

    mDynamicCastGet(IOStream*,curiostrm,curioobj)
    if ( locbut )
    {
	const bool canreloc = curiostrm && !curiostrm->implReadOnly();
	mSetTBStateAndTT4Cur( locbut, canreloc, tr("Relocate" ) );
    }
    mSetTBStateAndTT4Cur( renbut, curiostrm, uiStrings::sRename() );

#define mSetTBStateAndTT4Chosen(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt = toUiString("%1 %2").arg(oper).arg(mToUiStringTodo( \
						chosennames.getDispString(3)));\
    tb->setToolTip( tt )

    mDynamicCastGet(IOStream*,firstchoseniostrm,firstchosenioobj)
    const bool cantoggro = firstchoseniostrm
			&& firstchoseniostrm->implExists(true);
    const bool isro = cantoggro && firstchoseniostrm->implReadOnly();
    useAlternative( robut, !isro );
    mSetTBStateAndTT4Chosen( robut, cantoggro,
			     isro ? uiStrings::sUnlock() : uiStrings::sLock() );

    if ( rembut )
    {
	const bool canrm = firstchosenioobj;
	mSetTBStateAndTT4Chosen( rembut, canrm, uiStrings::sDelete() );
    }

    delete curioobj;
    delete firstchosenioobj;
}


void uiIOObjManipGroup::tbPush( CallBacker* c )
{
    mDynamicCastGet(uiToolButton*,tb,c)
    if ( !tb )
	{ pErrMsg("CallBacker is not uiToolButton!"); return; }
    const MultiID curid = subj_.currentID();
    if ( curid.isUdf() )
	return;

    const bool isreloc = tb == locbut;
    const bool issetro = tb == robut;
    const bool isrename = tb == renbut;
    const bool isremove = tb == rembut;
    const bool issingle = isreloc || isrename;

    TypeSet<MultiID> chosenids;
    if ( !issingle )
	subj_.getChosenIDs( chosenids );
    IOObj* firstioobj = IOM().get( issingle ? curid : chosenids[0] );
    if ( !firstioobj )
	return;

    mDynamicCastGet(uiMainWin*,mw,subj_.obj_->mainwin())
    uiMsgMainWinSetter mws( mw );

    PtrMan<Translator> trans = firstioobj->createTranslator();
    bool chgd = false;
    if ( isreloc )
	chgd = relocEntry( *firstioobj, trans );
    else if ( isrename )
	chgd = renameEntry( *firstioobj, trans );
    else
    {
	ObjectSet<IOObj> ioobjs;
	for ( int idx=0; idx<chosenids.size(); idx++ )
	    ioobjs += IOM().get( chosenids[idx] );

	if ( issetro )
	{
	    const bool isro = trans ? trans->implReadOnly(firstioobj)
				    : firstioobj->implReadOnly();
	    for ( int idx=0; idx<ioobjs.size(); idx++ )
		readonlyEntry( *ioobjs[idx], trans, !isro );
	}
	else if ( isremove )
	{
	    if ( !ioobjs.size() )
		return;

	    const bool res = ioobjs.size()>1 ?	rmEntries( ioobjs )
					     : rmEntry( *ioobjs[0] );
	    if ( !chgd && res )
		chgd = res;
	}

	deepErase( ioobjs );
    }

    delete firstioobj;
    if ( chgd )
	subj_.chgsOccurred();
}


bool uiIOObjManipGroup::renameEntry(IOObj& ioobj, Translator* trans)
{
    uiString titl = toUiString("%1 '%2'").arg(uiStrings::sRename())
					       .arg(ioobj.uiName());
    uiGenInputDlg dlg( this, titl, mJoinUiStrs(sNew(), sName()),
			new StringInpSpec(ioobj.name()) );
    if ( !dlg.go() ) return false;

    BufferString newnm = dlg.text();
    if ( subj_.names().isPresent(newnm) )
    {
	if ( newnm != ioobj.name() )
	    uiMSG().error(tr("Name already in use"));
	return false;
    }
    else
    {
	IOObj* lioobj = IOM().getLocal( newnm, ioobj.group() );
	if ( lioobj )
	{
	    uiString msg = tr("This name is already used by a %1 object")
			 .arg(lioobj->translator());
	    delete lioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    if ( !IOM().implRename(ioobj.key(), newnm) )
	return false;

    return true;
}


bool uiIOObjManipGroup::rmEntry( IOObj& ioobj )
{
    const MultiID& key = ioobj.key();
    const bool exists = IOM().implExists( key );
    const bool readonly = IOM().isReadOnly( key );
    bool shldrm = !IOM().implIsLink( key );
    if ( exists && readonly && shldrm )
    {
	uiString msg = tr("'%1' is not writable; the actual data "
			  "will not be deleted.\nThe entry will only "
			  "disappear from the list.\nContinue?")
		     .arg(ioobj.name());
	if ( !uiMSG().askContinue(msg) )
	    return false;

	shldrm = false;
    }

    return exists ? uiIOObj(ioobj).removeImpl( true, shldrm )
		  : IOM().permRemove( ioobj.key() );
}


bool uiIOObjManipGroup::rmEntries( ObjectSet<IOObj>& ioobjs )
{
    if ( !ioobjs.size() )
	return false;

    uiString info = tr("Delete the following objects"
		       " from the database permanently?\n%1");

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    uiStringSet selnms;
    for (int idx = 0; idx<ioobjs.size(); idx++)
	selnms += ioobjs[idx]->uiName();

    info.arg( selnms.createOptionString(true,10,'\n') );
    if ( !uiMSG().askRemove( info ) )
	return false;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
	uiIOObj(*ioobjs[idx], true).removeImpl( true, true, false );

    return true;
}


bool uiIOObjManipGroup::relocEntry( IOObj& ioobj, Translator* trans )
{
    mDynamicCastGet(IOStream&,iostrm,ioobj)
    uiString caption = tr("New file location for '%1'").arg(ioobj.uiName());
    BufferString oldfnm( iostrm.fullUserExpr() );
    BufferString filefilt;
    BufferString defext( subj_.defExt() );
    if ( !defext.isEmpty() )
    {
	filefilt += "OpendTect Files (*."; filefilt += defext;
	filefilt += ");;";
    }
    filefilt += "All Files(*)";

    uiFileDialog dlg( this, uiFileDialog::Directory, oldfnm, filefilt, caption);
    if ( !dlg.go() ) return false;

    const char* newdir = dlg.fileName();
    if ( !File::isDirectory(newdir) )
    {
	uiMSG().error(tr("Selected location does not exist "
			 "or is not a folder."));
	return false;
    }
    if ( !IOM().implReloc(ioobj.key(), newdir) )
	return false;

    IOM().commitChanges( ioobj );
    return true;
}


bool uiIOObjManipGroup::readonlyEntry( IOObj& ioobj, Translator* trans,
				       bool set2ro )
{
    const bool exists = trans ? trans->implExists( &ioobj, true )
			      : ioobj.implExists( true );
    if ( !exists )
	return false;

    const bool oldreadonly = trans ? trans->implReadOnly( &ioobj )
				   : ioobj.implReadOnly();
    bool newreadonly = set2ro;
    if ( oldreadonly == newreadonly )
	return false;

    if (trans)
    {
	trans->implSetReadOnly( &ioobj, newreadonly );
	newreadonly = trans->implReadOnly( &ioobj );
    }
    else
    {
	ioobj.implSetReadOnly( newreadonly );
	newreadonly = ioobj.implReadOnly();
    }

    if ( oldreadonly == newreadonly )
	uiMSG().warning(tr("Could not change the read-only status"));

    selChg();
    return false;
}


void uiIOObjManipGroup::relocCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    subj_.relocStart( msg );
}


// uiIOObjManipGroupSubj
uiIOObjManipGroupSubj::uiIOObjManipGroupSubj( uiObject* obj )
    : obj_(obj)
{}


uiIOObjManipGroupSubj::~uiIOObjManipGroupSubj()
{}
