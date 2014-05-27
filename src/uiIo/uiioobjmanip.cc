/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjmanip.h"

#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "oddirs.h"
#include "pixmap.h"
#include "ptrman.h"
#include "transl.h"

#include "uibuttongroup.h"
#include "uifiledlg.h"
#include "uiioobj.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uitoolbutton.h"


uiManipButGrp::ButData::ButData( uiToolButton* b, const char* p, const char* t )
	: but(b)
	, pmnm(p)
	, tt(t)
{
}


uiToolButton* uiManipButGrp::addButton( Type tp, const char* tooltip,
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


uiToolButton* uiManipButGrp::addButton( const char* pmnm, const char* tooltip,
					const CallBack& cb )
{
    uiToolButton* button = new uiToolButton( this, pmnm, tooltip, cb );
    butdata += new ButData( button, pmnm, tooltip );
    altbutdata += 0;
    return button;
}


void uiManipButGrp::setAlternative( uiToolButton* button, const char* pm,
				    const char* tt )
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
	    button->setPixmap( ioPixmap(bd.pmnm) );
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
	locbut = addButton( FileLocation, "Change location on disk", cb );
    renbut = addButton( Rename, "Rename this object", cb );
    robut = addButton( ReadOnly, "Toggle Read only : locked", cb );
    setAlternative( robut, "unlock", "Toggle Read only : editable" );
    if ( withremove )
	rembut = addButton( Remove, "Remove selected", cb );
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

    BufferString tt;
#define mSetTBStateAndTT4Cur(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt.set( oper ).add(" '").add( curioobj->name() ).add("'"); \
    tb->setToolTip( tt )

    if ( locbut )
    {
	mDynamicCastGet(IOStream*,iostrm,curioobj)
	const bool canreloc = iostrm && !iostrm->implReadOnly();
	mSetTBStateAndTT4Cur( locbut, canreloc, "Relocate" );
    }
    mSetTBStateAndTT4Cur( renbut, true, "Rename" );

#define mSetTBStateAndTT4Chosen(tb,cond,oper) \
    tb->setSensitive( cond ); \
    if ( !cond ) \
	tt.setEmpty(); \
    else \
	tt.set( oper ).add(" ").add( chosennames.getDispString(3) ); \
    tb->setToolTip( tt )

    const bool cantoggro = firstchosenioobj
			&& firstchosenioobj->implExists(true);
    const bool isro = cantoggro && firstchosenioobj->implReadOnly();
    useAlternative( robut, !isro );
    mSetTBStateAndTT4Chosen( robut, cantoggro, isro ? "Unlock":"Lock" );

    if ( rembut )
    {
	const bool canrm = firstchosenioobj;
	mSetTBStateAndTT4Chosen( rembut, canrm, "Remove" );
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
    if ( curid.isEmpty() )
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

    PtrMan<Translator> tr = firstioobj->createTranslator();
    bool chgd = false;
    if ( isreloc )
	chgd = relocEntry( *firstioobj, tr );
    else if ( isrename )
	chgd = renameEntry( *firstioobj, tr );
    else
    {
	ObjectSet<IOObj> ioobjs;
	for ( int idx=0; idx<chosenids.size(); idx++ )
	    ioobjs += IOM().get( chosenids[idx] );

	if ( issetro )
	{
	    const bool isro = tr ? tr->implReadOnly(firstioobj)
				 : firstioobj->implReadOnly();
	    for ( int idx=0; idx<ioobjs.size(); idx++ )
		readonlyEntry( *ioobjs[idx], tr, !isro );
	}
	else if ( isremove )
	{
	    for ( int idx=0; idx<ioobjs.size(); idx++ )
	    {
		const bool res = rmEntry( *ioobjs[idx] );
		if ( !chgd && res )
		    chgd = res;
	    }
	}
	deepErase( ioobjs );
    }

    delete firstioobj;
    if ( chgd )
	subj_.chgsOccurred();
}


bool uiIOObjManipGroup::renameEntry( IOObj& ioobj, Translator* tr )
{
    BufferString titl( "Rename '" );
    titl += ioobj.name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
			new StringInpSpec(ioobj.name()) );
    if ( !dlg.go() ) return false;

    BufferString newnm = dlg.text();
    if ( subj_.names().isPresent(newnm) )
    {
	if ( newnm != ioobj.name() )
	    uiMSG().error( "Name already in use" );
	return false;
    }
    else
    {
	IOObj* lioobj = IOM().getLocal( newnm, ioobj.group() );
	if ( lioobj )
	{
	    BufferString msg( "This name is already used by a ",
				lioobj->translator(), " object" );
	    delete lioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    ioobj.setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,&ioobj)
    if ( iostrm )
    {
	if ( !iostrm->implExists(true) )
	    iostrm->genDefaultImpl();
	else
	{
	    IOStream chiostrm;
	    chiostrm.copyFrom( iostrm );
	    FilePath fp( iostrm->fileName() );
	    if ( tr )
		chiostrm.setExt( tr->defExtension() );

	    BufferString cleannm( chiostrm.name() );
	    cleannm.clean( BufferString::NoFileSeps );
	    chiostrm.setName( cleannm );
	    chiostrm.genDefaultImpl();
	    chiostrm.setName( newnm );

	    FilePath deffp( chiostrm.fileName() );
	    fp.setFileName( deffp.fileName() );
	    chiostrm.setFileName( fp.fullPath() );

	    const bool newfnm = chiostrm.fileName()!=iostrm->fileName();
	    if ( newfnm && !doReloc(tr,*iostrm,chiostrm) )
	    {
		if ( newnm.contains('/') || newnm.contains('\\') )
		{
		    newnm.clean( BufferString::AllowDots );
		    chiostrm.setName( newnm );
		    chiostrm.genDefaultImpl();
		    deffp.set( chiostrm.fileName() );
		    fp.setFileName( deffp.fileName() );
		    chiostrm.setFileName( fp.fullPath() );
		    chiostrm.setName( iostrm->name() );
		    if ( !doReloc(tr,*iostrm,chiostrm) )
			return false;
		}
	    }

	    iostrm->copyFrom( &chiostrm );
	}
    }

    IOM().commitChanges( ioobj );
    return true;
}


bool uiIOObjManipGroup::rmEntry( IOObj& ioobj )
{
    PtrMan<Translator> tr = ioobj.createTranslator();
    const bool exists = tr ? tr->implExists(&ioobj,true)
			   : ioobj.implExists(true);
    const bool readonly = tr ? tr->implReadOnly(&ioobj) : ioobj.implReadOnly();
    bool shldrm = tr ? tr->implShouldRemove(&ioobj) : ioobj.implShouldRemove();
    if ( exists && readonly && shldrm )
    {
	BufferString msg( "'", ioobj.name(), "' " );
	msg.add( "is not writable; the actual data will not be removed." )
	   .addNewLine()
	   .add( "The entry will only disappear from the list.\nContinue?" );
	if ( !uiMSG().askContinue(msg) )
	    return false;
	shldrm = false;
    }

    return exists ? uiIOObj(ioobj).removeImpl( true, shldrm )
		  : IOM().permRemove( ioobj.key() );
}


bool uiIOObjManipGroup::relocEntry( IOObj& ioobj, Translator* tr )
{
    mDynamicCastGet(IOStream&,iostrm,ioobj)
    BufferString caption( "New file location for '" );
    caption += ioobj.name(); caption += "'";
    BufferString oldfnm( iostrm.getExpandedName(true) );
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

    IOStream chiostrm;
    chiostrm.copyFrom( &iostrm );
    const char* newdir = dlg.fileName();
    if ( !File::isDirectory(newdir) )
    { uiMSG().error( "Selected path is not a directory" ); return false; }

    FilePath fp( oldfnm ); fp.setPath( newdir );
    chiostrm.setFileName( fp.fullPath() );
    if ( !doReloc(tr,iostrm,chiostrm) )
	return false;

    IOM().commitChanges( ioobj );
    return true;
}


bool uiIOObjManipGroup::readonlyEntry( IOObj& ioobj, Translator* tr,
				       bool set2ro )
{
    const bool exists = tr ? tr->implExists(&ioobj,true)
			   : ioobj.implExists(true);
    if ( !exists )
	return false;

    const bool oldreadonly = tr ? tr->implReadOnly(&ioobj)
				: ioobj.implReadOnly();
    bool newreadonly = set2ro;
    if ( oldreadonly == newreadonly )
	return false;

    if ( tr )
    {
	tr->implSetReadOnly( &ioobj, newreadonly );
	newreadonly = tr->implReadOnly( &ioobj );
    }
    else
    {
	ioobj.implSetReadOnly( newreadonly );
	newreadonly = ioobj.implReadOnly();
    }

    if ( oldreadonly == newreadonly )
	uiMSG().warning( "Could not change the read-only status" );

    selChg();
    return false;
}


bool uiIOObjManipGroup::doReloc( Translator* tr, IOStream& iostrm,
				 IOStream& chiostrm )
{
    const bool oldimplexist = tr ? tr->implExists(&iostrm,true)
				 : iostrm.implExists(true);
    BufferString newfname( chiostrm.getExpandedName(true) );

    bool succeeded = true;
    if ( oldimplexist )
    {
	const bool newimplexist = tr ? tr->implExists(&chiostrm,true)
				     : chiostrm.implExists(true);
	if ( newimplexist && !uiIOObj(chiostrm).removeImpl(false,true) )
	    return false;

	CallBack cb( mCB(this,uiIOObjManipGroup,relocCB) );
	succeeded = tr  ? tr->implRename( &iostrm, newfname, &cb )
			: iostrm.implRename( newfname, &cb );
    }

    if ( succeeded )
	iostrm.setFileName( newfname );
    else
	uiMSG().error( "Relocation failed" );
    return succeeded;
}


void uiIOObjManipGroup::relocCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    subj_.relocStart( msg );
}
