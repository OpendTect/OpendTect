/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjmanip.cc,v 1.6 2003-11-07 12:22:01 bert Exp $
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
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "transl.h"
#include "ptrman.h"
#include "filegen.h"
#include "errh.h"


uiIOObjManipGroup::uiIOObjManipGroup( uiListBox* l, IODirEntryList& el,
       				      const char* de )
    	: uiButtonGroup(l->parent(),"")
	, box(l)
	, entries(el)
	, defext(de)
    	, preRelocation(this)
    	, postRelocation(this)
{
    const ioPixmap locpm( GetDataFileName("filelocation.png") );
    const ioPixmap renpm( GetDataFileName("renameobj.png") );
    const ioPixmap rempm( GetDataFileName("trashcan.png") );
    const ioPixmap ropm( GetDataFileName("readonly.png") );
    locbut = new uiToolButton( this, "File loc TB", locpm );
    renbut = new uiToolButton( this, "Obj rename TB", renpm );
    robut = new uiToolButton( this, "Readonly togg", ropm );
    rembut = new uiToolButton( this, "Remove", rempm );
    robut->setToggleButton( true );
    locbut->activated.notify( mCB(this,uiIOObjManipGroup,tbPush) );
    renbut->activated.notify( mCB(this,uiIOObjManipGroup,tbPush) );
    robut->activated.notify( mCB(this,uiIOObjManipGroup,tbPush) );
    rembut->activated.notify( mCB(this,uiIOObjManipGroup,tbPush) );
    locbut->setToolTip( "Change location on disk" );
    renbut->setToolTip( "Rename this object" );
    robut->setToolTip( "Toggle read-only" );
    rembut->setToolTip( "Remove this object" );
    attach( rightOf, box );
}


void uiIOObjManipGroup::selChg( CallBacker* c )
{
    ioobj = entries.selected();
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
    ioobj = entries.selected();
    if ( !ioobj ) return;
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
	box->addItem( entries[idx]->name() );
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
	IOObj* ioobj = IOM().getLocal( newnm );
	if ( ioobj )
	{
	    BufferString msg( "This name is already used by a " );
	    msg += ioobj->translator();
	    msg += " object";
	    delete ioobj;
	    uiMSG().error( msg );
	    return false;
	}
    }

    ioobj->setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm && iostrm->implExists(true) )
    {
	IOStream chiostrm;
	chiostrm.copyFrom( iostrm );
	if ( tr )
	    chiostrm.setExt( tr->defExtension() );
	chiostrm.genDefaultImpl();
	if ( !doReloc(tr,*iostrm,chiostrm) )
	    return false;

	iostrm->copyFrom( &chiostrm );
    }

    IOM().commitChanges( *ioobj );
    return true;
}


bool uiRmIOObjImpl( IOObj& ioob, bool askexist )
{
    BufferString mess = "Remove ";
    if ( askexist ) mess += " existing ";
    mess += "'";
    if ( !ioob.isLink() )
	{ mess += ioob.fullUserExpr(true); mess += "'?"; }
    else
    {
	FileNameString fullexpr( ioob.fullUserExpr(true) );
	mess += File_getFileName(fullexpr);
	mess += "'\n- and everything in it! - ?";
    }
    if ( !uiMSG().askGoOn(mess) )
	return false;

    if ( !fullImplRemove(ioob) )
    {
	BufferString mess = "Could not remove '";
	mess += ioob.fullUserExpr(true);
	mess += "'\nRemove entry from list anyway?";
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

    const char* filenm = File_getFileName(oldfnm);
    BufferString newfnm = File_getFullPath( newdir, filenm );
    chiostrm.setFileName( newfnm );
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
