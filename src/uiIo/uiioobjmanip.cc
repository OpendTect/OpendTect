/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjmanip.cc,v 1.12 2004-11-02 13:56:44 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjmanip.h"
#include "iodirentry.h"
#include "uilistbox.h"
#include "uifiledlg.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"
#include "ptrman.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "transl.h"
#include "ptrman.h"
#include "filegen.h"
#include "filepath.h"
#include "errh.h"


uiToolButton* uiManipButGrp::addButton( Type tp, const CallBack& cb,
					const char* tooltip )
{
    PtrMan<ioPixmap> pm = 0;
    switch ( tp )
    {
	case FileLocation:
	    pm = new ioPixmap( GetDataFileName("filelocation.png") ); break;
	case Rename:
	    pm = new ioPixmap( GetDataFileName("renameobj.png") ); break;
	case Remove:
	    pm = new ioPixmap( GetDataFileName("trashcan.png") ); break;
	case ReadOnly:
	    pm = new ioPixmap( GetDataFileName("readonly.png") ); break;
    }

    return addButton( *pm, cb, tooltip );
}


uiToolButton* uiManipButGrp::addButton( const ioPixmap& pm, const CallBack& cb,
					const char* tooltip )
{
    uiToolButton* button = new uiToolButton( this, "ToolButton", pm, cb );
    button->setToolTip( tooltip );
    return button;
}



uiIOObjManipGroup::uiIOObjManipGroup( uiListBox* l, IODirEntryList& el,
       				      const char* de )
    	: uiManipButGrp( l->parent() )
	, box(l)
	, entries(el)
	, defext(de)
    	, preRelocation(this)
    	, postRelocation(this)
    	, ioobj(0)
{
    const CallBack cb( mCB(this,uiIOObjManipGroup,tbPush) );
    locbut = addButton( FileLocation, cb, "Change location on disk" );
    renbut = addButton( Rename, cb, "Rename this object" );
    robut = addButton( ReadOnly, cb, "Toggle read-only" );
    rembut = addButton( Remove, cb, "Remove this object" );
    robut->setToggleButton( true );
    attach( rightOf, box );
}


uiIOObjManipGroup::~uiIOObjManipGroup()
{
    delete ioobj;
}


bool uiIOObjManipGroup::gtIOObj()
{
    delete ioobj;
    ioobj = entries.selected();
    if ( !ioobj ) return false;
    ioobj = ioobj->clone();
    return true;
}


void uiIOObjManipGroup::selChg( CallBacker* c )
{
    if ( !gtIOObj() ) return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    const bool isexisting = ioobj && ioobj->implExists(true);
    const bool isreadonly = isexisting && ioobj->implReadOnly();
    locbut->setSensitive( iostrm && !isreadonly );
    robut->setOn( isreadonly );
    renbut->setSensitive( ioobj );
    rembut->setSensitive( ioobj && !isreadonly );
}


void uiIOObjManipGroup::tbPush( CallBacker* c )
{
    if ( !gtIOObj() ) return;
    mDynamicCastGet(uiToolButton*,tb,c)
    if ( !tb ) { pErrMsg("CallBacker is not uiToolButton!"); return; }

    const int curitm = box->currentItem();
    MultiID prevkey( ioobj->key() );
    PtrMan<Translator> tr = ioobj->getTranslator();

    bool chgd = false;
    if ( tb == locbut )
	chgd = relocEntry( tr );
    else if ( tb == robut )
	chgd = readonlyEntry( tr );
    else if ( tb == renbut )
	chgd = renameEntry( tr );
    else if ( tb == rembut )
    {
	bool exists = tr ? tr->implExists(ioobj,true) : ioobj->implExists(true);
	bool readonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
	if ( exists && readonly )
	{
	    uiMSG().error( "Entry is not writable.\nPlease change this first.");
	    return; 
	}
	chgd = rmEntry( exists );
	if ( chgd )
	{
	    entries.fill( IOM().dirPtr() );
	    prevkey = "";
	    const int newcur = curitm >= entries.size()
			     ? entries.size() - 1 : curitm;
	    if ( newcur >= 0 )
		prevkey = entries[newcur]->ioobj->key();
	}
    }

    if ( chgd )
    {
	refreshList( prevkey );
	selChg(c);
	if ( tb == locbut ) postRelocation.trigger();
    }
}


void uiIOObjManipGroup::refreshList( const MultiID& key )
{
    entries.fill( IOM().dirPtr() );
    if ( key != "" )
	entries.setSelected( key );

    box->empty();
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	IODirEntry* entry = entries[idx];
	box->addItem( entries[idx]->name() );
	if ( entry->ioobj == entries.selected() )
	    box->setCurrentItem( idx );
    }

}


bool uiIOObjManipGroup::renameEntry( Translator* tr )
{
    BufferString titl( "Rename '" );
    titl += ioobj->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
	    		new StringInpSpec(ioobj->name()) );
    if ( !dlg.go() ) return false;

    BufferString newnm = dlg.text();
    if ( box->isPresent(newnm) )
    {
	if ( newnm != ioobj->name() )
	    uiMSG().error( "Name already in use" );
	return false;
    }
    else
    {
	IOObj* lioobj = IOM().getLocal( newnm );
	if ( lioobj )
	{
	    BufferString msg( "This name is already used by a " );
	    msg += lioobj->translator();
	    msg += " object";
	    delete lioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    ioobj->setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
    {
	if ( !iostrm->implExists(true) )
	    iostrm->genDefaultImpl();
	else
	{
	    IOStream chiostrm;
	    chiostrm.copyFrom( iostrm );
	    if ( tr )
		chiostrm.setExt( tr->defExtension() );
	    chiostrm.genDefaultImpl();
	    if ( !doReloc(tr,*iostrm,chiostrm) )
	    {
		if ( strchr(newnm.buf(),'/') || strchr(newnm.buf(),'\\') )
		{
		    cleanupString(newnm.buf(),NO,NO,YES);
		    chiostrm.setName( newnm );
		    chiostrm.genDefaultImpl();
		    chiostrm.setName( iostrm->name() );
		    if ( !doReloc(tr,*iostrm,chiostrm) )
			return false;
		}
	    }

	    iostrm->copyFrom( &chiostrm );
	}
    }

    IOM().commitChanges( *ioobj );
    return true;
}


bool uiRmIOObjImpl( IOObj& ioob, bool askexist )
{
    BufferString mess = "Remove ";
    if ( askexist ) mess += " existing ";
    mess += " data file(s), at\n'";
    if ( !ioob.isLink() )
    {
	mess += ioob.fullUserExpr(true);
	mess += "'?";
    }
    else
    {
	FileNameString fullexpr( ioob.fullUserExpr(true) );
	mess += FilePath(fullexpr).fileName();
	mess += "'\n- and everything in it! - ?";
    }
    if ( !uiMSG().askGoOn(mess) )
	return false;

    if ( !fullImplRemove(ioob) )
    {
	BufferString mess = "Could not remove data file(s).\n";
	mess += "Remove entry from list anyway?";
	if ( !uiMSG().askGoOn(mess) )
	    return false;
    }
    return true;
}


bool uiIOObjManipGroup::rmEntry( bool rmabl )
{
    if ( rmabl && !uiRmIOObjImpl( *ioobj, false ) )
	return false;

    entries.curRemoved();
    IOM().permRemove( ioobj->key() );
    return true;
}


bool uiIOObjManipGroup::relocEntry( Translator* tr )
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    BufferString caption( "New file/location for '" );
    caption += ioobj->name(); caption += "'";
    BufferString oldfnm( iostrm->fullUserExpr(true) );
    BufferString filefilt( "*" );
    if ( defext != "" )
    {
	filefilt += "."; filefilt += defext;
	filefilt += ";;*";
    }

    uiFileDialog dlg( this, uiFileDialog::Directory, oldfnm, filefilt, caption);
    if ( !dlg.go() ) return false;

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    const char* newdir = dlg.fileName();
    if ( !File_isDirectory(newdir) )
    { uiMSG().error( "Selected path is not a directory" ); return false; }

    FilePath fp( oldfnm ); fp.setPath( newdir );
    chiostrm.setFileName( fp.fullPath() );
    if ( !doReloc(tr,*iostrm,chiostrm) )
	return false;

    IOM().commitChanges( *iostrm );
    return true;
}


bool uiIOObjManipGroup::readonlyEntry( Translator* tr )
{
    if ( !ioobj ) { pErrMsg("Huh"); return false; }

    bool exists = tr ? tr->implExists(ioobj,true) : ioobj->implExists(true);
    if ( !exists ) return false;

    bool oldreadonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
    bool newreadonly = robut->isOn();
    if ( oldreadonly == newreadonly ) return false;

    bool res = tr ? tr->implSetReadOnly(ioobj,newreadonly)
		: ioobj->implSetReadOnly(newreadonly);

    newreadonly = tr ? tr->implReadOnly(ioobj) : ioobj->implReadOnly();
    if ( oldreadonly == newreadonly )
	uiMSG().warning( "Could not change the read-only status" );

    selChg(0);
    return false;
}


bool uiIOObjManipGroup::doReloc( Translator* tr, IOStream& iostrm,
				 IOStream& chiostrm )
{
    const bool oldimplexist = tr ? tr->implExists(&iostrm,true)
				 : iostrm.implExists(true);
    BufferString newfname( chiostrm.fullUserExpr(true) );

    if ( oldimplexist )
    {
	const bool newimplexist = tr ? tr->implExists(&chiostrm,true)
				     : chiostrm.implExists(true);
	if ( newimplexist && !uiRmIOObjImpl( chiostrm, true ) )
	    return false;

	CallBack cb( mCB(this,uiIOObjManipGroup,relocCB) );
	if ( tr )
	    tr->implRename( &iostrm, newfname, &cb );
	else
	    iostrm.implRename( newfname, &cb );
    }

    iostrm.setFileName( newfname );
    return true;
}


void uiIOObjManipGroup::relocCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    relocmsg = msg;
    preRelocation.trigger();
}
