/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseisfileman.cc,v 1.4 2002-06-14 09:22:38 bert Exp $
________________________________________________________________________

-*/


#include "uiseisfileman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uimergeseis.h"
#include "uifiledlg.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "filegen.h"
#include "binidselimpl.h"
#include "ptrman.h"
#include "transl.h"


uiSeisFileMan::uiSeisFileMan( uiParent* p )
        : uiDialog(p,uiDialog::Setup("Seismic file management",
                                     "Remove, rename, locate seismics",
                                     "0"))
	, ctio(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
	, ioobj(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    ctio.ctxt.trglobexpr = "CBVS";
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    listfld = new uiListBox( this, entrylist->Ptr() );
    listfld->setHSzPol( uiObject::medvar );
    listfld->selectionChanged.notify( mCB(this,uiSeisFileMan,selChg) );

    rembut = new uiPushButton( this, "Remove ..." );
    rembut->activated.notify( mCB(this,uiSeisFileMan,removePush) );
    rembut->attach( rightOf, listfld );
    renamebut = new uiPushButton( this, "Rename ..." );
    renamebut->activated.notify( mCB(this,uiSeisFileMan,renamePush) );
    renamebut->attach( alignedBelow, rembut );
    relocbut = new uiPushButton( this, "Location ..." );
    relocbut->activated.notify( mCB(this,uiSeisFileMan,relocatePush) );
    relocbut->attach( alignedBelow, renamebut );
    mergebut = new uiPushButton( this, "Merge ..." );
    mergebut->activated.notify( mCB(this,uiSeisFileMan,mergePush) );
    mergebut->attach( alignedBelow, relocbut );
  
    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( alignedBelow, listfld );
    infofld->setPrefHeightInChar( 5 );
    infofld->setPrefWidthInChar( 40 );

    selChg( this ); 
    setCancelText( "" );
}


void uiSeisFileMan::selChg( CallBacker* )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();
    mkFileInfo();
}


void uiSeisFileMan::mkFileInfo()
{
    if ( !ioobj )
    {
	infofld->setText( "" );
	return;
    }

    BufferString txt;
    BinIDSampler bs;
    StepInterval<float> zrg;
    if ( SeisTrcTranslator::getRanges( *ioobj, bs, zrg ) )
    {
	txt = "Inline range: "; txt += bs.start.inl; txt += " - "; 
			        txt += bs.stop.inl;
	txt += "\nCrossline range: "; txt += bs.start.crl; txt += " - "; 
			           txt += bs.stop.crl;
	txt += "\nZ-range: "; txt += zrg.start; txt += " - "; 
			        txt += zrg.stop;
    }

    if ( ioobj->pars().size() && ioobj->pars().hasKey("Type") )
    {
	txt += "\nType: "; txt += ioobj->pars().find( "Type" );
    }

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
    {
	BufferString fname( iostrm->fileName() );
	if ( !File_isAbsPath(fname) )
	{
	    fname = GetDataDir();
	    fname = File_getFullPath( fname, "Seismics" );
	    fname = File_getFullPath( fname, iostrm->fileName() );
	}
	txt += "\nLocation: "; txt += File_getPathOnly(fname);
	txt += "\nFile name: "; txt += File_getFileName(fname);
    }
    
    infofld->setText( txt );
}


void uiSeisFileMan::refreshList( int curitm )
{
    entrylist->fill( IOM().dirPtr() );
    if ( curitm >= entrylist->size() ) curitm--;
    entrylist->setCurrent( curitm );
    ioobj = entrylist->selected();
    listfld->empty();
    listfld->addItems( entrylist->Ptr() );
}


void uiSeisFileMan::removePush( CallBacker* )
{
    if ( !ioobj ) return;
    int curitm = listfld->currentItem();
    if ( ioobj->implRemovable() )
    {
	BufferString msg( "Remove '" );
	if ( !ioobj->isLink() )
	    { msg += ioobj->fullUserExpr(YES); msg += "'?"; }
	else
	{
	    FileNameString fullexpr( ioobj->fullUserExpr(YES) );
	    msg += File_getFileName(fullexpr);
	    msg += "'\n- and everything in it! - ?";
	}
	if ( !uiMSG().askGoOn(msg) ) return;

	PtrMan<Translator> tr = ioobj->getTranslator();
	bool rmd = tr ? tr->implRemove(ioobj) : ioobj->implRemove();
	if ( !rmd )
	{
	    msg = "Could not remove '";
	    msg += ioobj->fullUserExpr(YES); msg += "'";
	    uiMSG().warning( msg );
	}
    }

    entrylist->curRemoved();
    IOM().removeAux( ioobj->key() );
    IOM().dirPtr()->permRemove( ioobj->key() );
    refreshList( curitm );
}


void uiSeisFileMan::renamePush( CallBacker* )
{
    if ( !ioobj ) return;
    BufferString fulloldname = ioobj->fullUserExpr(true);
    int curitm = listfld->currentItem();
    BufferString filenm = listfld->getText();
    char* ptr = filenm.buf();
    skipLeadingBlanks( ptr );
    FileNameDlg dlg( this, ptr );
    if ( dlg.go() )
    {
	if ( listfld->isPresent( dlg.getNewName() ) )
	    if ( !uiMSG().askGoOn("Filename exists, overwrite?") )
		return;

	if ( IOM().setFileName( ioobj->key(), dlg.getNewName() ) )
	{
	    refreshList( curitm );
	    handleMultiFiles( fulloldname, ioobj->fullUserExpr(true) );
	}
    }
}


void uiSeisFileMan::relocatePush( CallBacker* )
{
    if ( !ioobj) return;
    int curitm = listfld->currentItem();
    const FileNameString fulloldname = ioobj->fullUserExpr(true);
    const char* dirpath = File_getPathOnly( fulloldname );
    BufferString fname = File_getFileName( fulloldname );
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, dirpath );
    if ( dlg.go() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	if ( !iostrm ) return;
        BufferString newdirpath = dlg.fileName();

	BufferString fullnewname = File_getFullPath(newdirpath,fname);
	if ( File_exists(fullnewname) )
	{
	    uiMSG().error( "New name exists at given location\n"
			   "Please select another location" );
	    return;
	}
	UsrMsg( "Moving cube ..." );
	if ( !File_rename(fulloldname,fullnewname) )
	{
	    uiMSG().error( "Could not move file to new location" );
	    UsrMsg( "" );
	    return;
	}
	iostrm->setFileName( fullnewname );
	if ( !handleMultiFiles( fulloldname, fullnewname ) ) return;

	if ( IOM().dirPtr()->commitChanges( ioobj ) )
	    refreshList( curitm );
    }
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    int curitm = listfld->currentItem();
    uiMergeSeis dlg( this );
    dlg.go();
    refreshList( curitm );
}

   
bool uiSeisFileMan::handleMultiFiles( const char* oldnm, const char* newnm )
{
    BufferString oldfname( oldnm );
    oldfname = File_getFileName( oldfname );
    BufferString oldpathname( oldnm );
    oldpathname = File_getPathOnly( oldpathname );

    BufferString newfname( newnm );
    newfname = File_getFileName( newfname );
    BufferString newpathname( newnm );
    newpathname = File_getPathOnly( newpathname );

    char* ptr1 = strrchr( oldfname.buf(), '.' );
    BufferString ext;
    if ( ptr1 )
        { ext = ptr1; *ptr1 = '\0'; }

    char* ptr2 = strrchr( newfname.buf(), '.' );
    if ( ptr2 ) *ptr2 = '\0';

    ObjectSet<BufferString> oldfnms;
    oldfnms += new BufferString( oldnm );
    ObjectSet<BufferString> newfnms;
    newfnms += new BufferString( newnm );
    for ( int idx=1; ; idx++ )
    {
	oldfname += idx < 10 ? "^0" : "^";
	oldfname += idx;
	if ( ptr1 ) oldfname += ext;
	BufferString fulloldname = File_getFullPath( oldpathname, oldfname );
	if ( !File_exists((const char*)fulloldname) )
	    return true;
	newfname += idx < 10 ? "^0" : "^";
	newfname += idx;
	if ( ptr2 ) newfname += ext;
	BufferString fullnewname = File_getFullPath( newpathname, newfname );
	int res = File_rename( fulloldname, fullnewname );
	if ( !res )
	{ 
	    createLinks( oldfnms, newfnms ); 
	    return false; 
	}
	oldfnms += new BufferString( fulloldname );
	newfnms += new BufferString( fullnewname );
    }
}


void uiSeisFileMan::createLinks( ObjectSet<BufferString>& oldnms, 
				 ObjectSet<BufferString>& newnms )
{
    for ( int idx=0; idx<oldnms.size(); idx++ )
    {
	File_createLink( newnms[idx]->buf(), oldnms[idx]->buf() );
    }
}


FileNameDlg::FileNameDlg( uiParent* p, const char* name_ )
        : uiDialog(p,uiDialog::Setup("Seismic", "Rename object"))
{   namefld = new uiGenInput( this, "Name", StringInpSpec(name_) ); }

const char* FileNameDlg::getNewName()
{   return namefld->text(); }


