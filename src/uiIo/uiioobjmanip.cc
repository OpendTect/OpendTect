/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uiioobjmanip.h"

#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "oddirs.h"
#include "ptrman.h"
#include "transl.h"

#include "uibuttongroup.h"
#include "uifileselector.h"
#include "uigeninputdlg.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uitoolbutton.h"


uiManipButGrp::ButData::ButData( uiToolButton* b, const char* p,
				 const uiString& t )
	: but(b)
	, pmnm(p)
	, tt(t)
{
}


uiToolButton* uiManipButGrp::addButton( Type tp, const uiString& tooltip,
					const CallBack& cb )
{
    const char* pm = 0;
    switch ( tp )
    {
	case FileLocation:	pm = "filelocation";	break;
	case Rename:		pm = "renameobj";	break;
	case Remove:		pm = "delete";		break;
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



void uiManipButGrp::remove( uiToolButton* tb )
{
    for ( int idx=0; idx<butdata.size(); idx++ )
    {
	if ( butdata[idx]->but == tb )
	{
	    delete butdata.removeSingle( idx );
	    delete altbutdata.removeSingle( idx );
	    removeButton( tb );
	}
    }
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


uiIOObjManipGroupSubj::~uiIOObjManipGroupSubj()
{
    detachAllNotifiers();
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
	rembut = addButton( Remove,
			uiStrings::phrDelete(uiStrings::sSelected()), cb );
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
    const DBKey curid = subj_.currentID();
    IOObj* curioobj = curid.getIOObj();
    if ( !curioobj )
    {
	renbut->setSensitive( false );
	robut->setSensitive( false );
	if ( locbut ) locbut->setSensitive( false );
	if ( rembut ) rembut->setSensitive( false );
	return;
    }

    DBKeySet chosenids; subj_.getChosenIDs( chosenids );
    BufferStringSet chosennames; subj_.getChosenNames( chosennames );
    if ( chosenids.isEmpty() )
	{ delete curioobj; return; }

    IOObj* firstchosenioobj = chosenids[0].getIOObj();

    uiString tt;
#define mSetTBStateAndTT4Cur(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt = toUiString("%1 '%2'").arg(oper).arg(curioobj->name()); \
    tb->setToolTip( tt )

    mDynamicCastGet(IOStream*,curiostrm,curioobj)
    if ( locbut )
    {
	const bool canreloc = curiostrm && !curiostrm->implReadOnly();
	mSetTBStateAndTT4Cur( locbut, canreloc, uiStrings::sMove() );
    }
    mSetTBStateAndTT4Cur( renbut, curiostrm, uiStrings::sRename() );

#define mSetTBStateAndTT4Chosen(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt = toUiString("%1 %2").arg(oper).arg(toUiString( \
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
    const DBKey curid = subj_.currentID();
    if ( curid.isInvalid() )
	return;

    const bool isreloc = tb == locbut;
    const bool issetro = tb == robut;
    const bool isrename = tb == renbut;
    const bool isremove = tb == rembut;
    const bool issingle = isreloc || isrename;

    DBKeySet chosenids;
    if ( !issingle )
	subj_.getChosenIDs( chosenids );
    IOObj* firstioobj = (issingle ? curid : chosenids[0]).getIOObj();
    if ( !firstioobj )
	return;

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
	    ioobjs += chosenids[idx].getIOObj();

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


bool uiIOObjManipGroup::renameEntry( IOObj& ioobj, Translator* trans )
{
    uiString titl = toUiString("%1 '%2'").arg(uiStrings::sRename())
					       .arg(ioobj.name());
    uiGenInputDlg dlg( this, titl, tr("New Name"),
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
	IOObj* existioobj = DBM().getByName( newnm, ioobj.group() );
	if ( existioobj )
	{
	    uiString msg = tr("This name is already used by a %1 object")
			 .arg(existioobj->translator());
	    delete existioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    ioobj.setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,&ioobj)
    if ( iostrm )
    {
	if ( !iostrm->implExists(true) )
	    iostrm->genFileName();
	else
	{
	    IOStream chiostrm;
	    chiostrm.copyFrom( *iostrm );
	    File::Path fp( iostrm->fileSpec().fileName() );
	    if ( trans )
		chiostrm.setExt( trans->defExtension() );

	    BufferString cleannm( chiostrm.name() );
	    cleannm.clean( BufferString::NoFileSeps );
	    chiostrm.setName( cleannm );
	    chiostrm.genFileName();
	    chiostrm.setName( newnm );

	    File::Path deffp( chiostrm.fileSpec().fileName() );
	    fp.setFileName( deffp.fileName() );
	    chiostrm.fileSpec().setFileName( fp.fullPath() );

	    const bool newfnm = FixedString(chiostrm.fileSpec().fileName())
					    != iostrm->fileSpec().fileName();
	    if ( newfnm && !doReloc(trans,*iostrm,chiostrm) )
	    {
		if ( !newnm.contains('/') && !newnm.contains('\\') )
		    return false;

		newnm.clean( BufferString::AllowDots );
		chiostrm.setName( newnm );
		chiostrm.genFileName();
		deffp.set( chiostrm.fileSpec().fileName() );
		fp.setFileName( deffp.fileName() );
		chiostrm.fileSpec().setFileName( fp.fullPath() );
		chiostrm.setName( iostrm->name() );
		if (!doReloc(trans, *iostrm, chiostrm))
		    return false;
	    }

	    iostrm->copyFrom( chiostrm );
	}
    }

    ioobj.commitChanges();
    return true;
}


bool uiIOObjManipGroup::rmEntry( IOObj& ioobj )
{
    PtrMan<Translator> trans = ioobj.createTranslator();
    const bool exists = trans ? trans->implExists(&ioobj, true)
			      : ioobj.implExists(true);
    const bool readonly = trans ? trans->implReadOnly(&ioobj)
				: ioobj.implReadOnly();
    bool shldrm = trans ? !trans->implManagesObjects(&ioobj)
		        : !ioobj.implManagesObjects();
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
		  : ioobj.removeFromDB();
}


bool uiIOObjManipGroup::rmEntries( ObjectSet<IOObj>& ioobjs )
{
    if ( !ioobjs.size() )
	return false;

    uiString info = tr("Delete the following objects"
		       " from the database permanently?\n%1");

    uiStringSet selnms;
    for (int idx = 0; idx<ioobjs.size(); idx++)
	selnms += toUiString(ioobjs[idx]->name());

    info.arg( selnms.createOptionString(true,10,true) );
    if ( !uiMSG().askRemove( info ) )
	return false;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
	uiIOObj(*ioobjs[idx], true).removeImpl( true, true, false );

    return true;
}


bool uiIOObjManipGroup::relocEntry( IOObj& ioobj, Translator* trans )
{
    mDynamicCastGet(IOStream&,iostrm,ioobj)
    uiString caption = tr("New file location for '%1'").arg(ioobj.name());
    BufferString oldfnm( iostrm.mainFileName() );

    uiFileSelector::Setup fssu( oldfnm );
    fssu.selectDirectory();
    uiFileSelector uifs( this, fssu );
    uifs.caption() = caption;
    if ( !uifs.go() )
	return false;

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    const char* newdir = uifs.fileName();
    if ( !File::isDirectory(newdir) )
	{ uiMSG().error(tr("Selected path is not a directory")); return false; }

    File::Path fp( oldfnm ); fp.setPath( newdir );
    chiostrm.fileSpec().setFileName( fp.fullPath() );
    if (!doReloc(trans, iostrm, chiostrm))
	return false;

    ioobj.commitChanges();
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
	uiMSG().warning(tr("Cannot change the read-only status"));

    selChg();
    return false;
}


bool uiIOObjManipGroup::doReloc(Translator* trans, IOStream& iostrm,
				 IOStream& chiostrm )
{
    const bool oldimplexist = trans ? trans->implExists( &iostrm, true )
				    : iostrm.implExists( true );
    const BufferString newfname( chiostrm.mainFileName() );

    bool succeeded = true;
    if ( oldimplexist )
    {
	const bool newimplexist = trans ? trans->implExists(&chiostrm, true)
				        : chiostrm.implExists(true);
	if ( newimplexist && !uiIOObj(chiostrm).removeImpl(false,true) )
	    return false;

	CallBack cb( mCB(this,uiIOObjManipGroup,relocCB) );
	succeeded = trans ? trans->implRename( &iostrm, newfname, &cb )
			  : iostrm.implRename( newfname, &cb );
    }

    if ( succeeded )
	iostrm.fileSpec().setFileName( newfname );
    else
	uiMSG().error(tr("Relocation failed"));
    return succeeded;
}


void uiIOObjManipGroup::relocCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    subj_.relocStart( msg );
}
