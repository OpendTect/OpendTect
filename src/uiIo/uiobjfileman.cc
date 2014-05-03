/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiobjfileman.h"

#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "streamconn.h"
#include "strmprov.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "transl.h"


static const int cPrefHeight = 10;
static const int cPrefWidth = 75;


uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    const IOObjContext& ctxt )
    : uiDialog(p,s)
    , curioobj_(0)
    , ctxt_(*new IOObjContext(ctxt))
    , lastexternal_(0)
    , curimplexists_(false)
{
    setCtrlStyle( CloseOnly );
}


uiObjFileMan::~uiObjFileMan()
{
    delete curioobj_;
    delete &ctxt_;
}


void uiObjFileMan::createDefaultUI( bool needreloc, bool needremove,
				    bool multisel )
{
    listgrp_ = new uiGroup( this, "List Group" );
    IOM().to( ctxt_.getSelKey(), true );
    selgrp_ = new uiIOObjSelGrp( listgrp_, CtxtIOObj(ctxt_), 0, multisel,
				 needreloc, true, needremove );
    selgrp_->selectionChg.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    infogrp_ = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    uiToolButton* dummytb = new uiToolButton( infogrp_, uiIcon::None(),
					      "", CallBack() );
    dummytb->attach( rightTo, infofld_ );
    dummytb->display( false );

    uiGroup* notesgrp = new uiGroup( this, "Notes Group" );
    uiLabel* noteslbl = new uiLabel( notesgrp, "Notes:" );
    notesfld_ = new uiTextEdit( notesgrp, "User info" );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    notesfld_->setToolTip( "Notes" );
    notesfld_->attach( alignedBelow, noteslbl );
    uiToolButton* savebut = new uiToolButton( notesgrp, "save", "Save Notes",
	    mCB(this,uiObjFileMan,saveNotes) );
    savebut->attach( rightTo, notesfld_ );

    setPrefWidth( cPrefWidth );

    uiSplitter* sep = new uiSplitter( this, "List-Info splitter", false );
    sep->addGroup( listgrp_ );
    sep->addGroup( infogrp_ );
    sep->addGroup( notesgrp );
}


void uiObjFileMan::addTool( uiButton* but )
{
    if ( lastexternal_ )
	but->attach( rightOf, lastexternal_ );
    else
    {
	but->attach( ensureBelow, selgrp_ );
	infofld_->attach( ensureBelow, but );
    }

    lastexternal_ = but;
}


static BufferString getFileName( const IOObj& ioobj )
{
    FilePath fp( ioobj.fullUserExpr() );
    fp.setExtension( "note" );
    return fp.fullPath();
}


void uiObjFileMan::saveNotes( CallBacker* )
{
    BufferString txt = notesfld_->text();
    if ( !curioobj_ || txt.isEmpty() )
	return;

    StreamData sd = StreamProvider( getFileName(*curioobj_) ).makeOStream();
    if ( !sd.usable() )
	return;

    *sd.ostrm << txt << '\n';
    sd.close();
}


void uiObjFileMan::readNotes()
{
    if ( !curioobj_ )
    {
	notesfld_->setText( "" );
	return;
    }

    StreamData sd = StreamProvider( getFileName(*curioobj_) ).makeIStream();
    if ( !sd.usable() )
    {
	notesfld_->setText( "" );
	return;
    }

    BufferString note;
    char buf[1024];
    while ( sd.istrm->getline(buf,1024) )
    {
	if ( !note.isEmpty() )
	    note += "\n";
	note += buf;
    }

    sd.close();
    notesfld_->setText( note );
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    saveNotes(0);
    delete curioobj_;
    curioobj_ = selgrp_->nrSel() > 0 ? IOM().get(selgrp_->currentID()) : 0;
    curimplexists_ = curioobj_ && curioobj_->implExists(true);

    ownSelChg();
    if ( curioobj_ )
	mkFileInfo();
    else
	setInfo( "" );

    readNotes();
    BufferString msg;
    if ( curioobj_ )
	System::getFreeMBOnDiskMsg( System::getFreeMBOnDisk(*curioobj_), msg );
    toStatusBar( msg );
}


double uiObjFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    BufferString actualfilenm = File::isLink(filenm) ? File::linkTarget(filenm)
						     : filenm;
    if ( !File::exists(actualfilenm.buf()) )
	return 0;

    double ret = (double)File::getKbSize( actualfilenm.buf() );
    if ( !File::isDirectory(actualfilenm) )
    {
	nrfiles = 1;
	FilePath dirnm( actualfilenm );
	dirnm.setExtension( "" );
	if ( !File::isDirectory(dirnm.fullPath()) )
	    return ret;

	actualfilenm = dirnm.fullPath();
    }

    BufferStringSet filelist;
    File::makeRecursiveFileList( actualfilenm.buf(), filelist, true );
    for ( int idx=0; idx<filelist.size(); idx++ )
	ret += File::getKbSize( filelist.get(idx) );

    nrfiles += filelist.size();
    return ret;
}


BufferString uiObjFileMan::getFileSizeString( double filesz )
{
    BufferString szstr;
    if ( filesz > 1024 )
    {
        const bool doGb = filesz > 1048576;
	const int nr = doGb ? mNINT32(filesz/10485.76) : mNINT32(filesz/10.24);
	szstr = nr/100;
	const int rest = nr%100;
	szstr += rest < 10 ? ".0" : "."; szstr += rest;
	szstr += doGb ? " GB" : " MB";
    }
    else if ( filesz == 0 )
	szstr = "< 1 kB";
    else
    {
	szstr = filesz;
	szstr += " kB";
    }

    return szstr;
}


void uiObjFileMan::getTimeStamp( const char* fname,
				 BufferString& timestamp )
{
    timestamp = File::timeLastModified( fname );
    FilePath fp( fname );
    fp.setExtension( "" );
    const BufferString dirnm = fp.fullPath();
    if ( File::isDirectory(dirnm) )
	getTimeLastModified( dirnm, timestamp );
}


void uiObjFileMan::getTimeLastModified( const char* fname,
					BufferString& timestamp )
{
    const FixedString ftimestamp = File::timeLastModified( fname );
    if ( timestamp.isEmpty() || Time::isEarlier( timestamp, ftimestamp ) )
	timestamp = ftimestamp;

    if ( !File::isDirectory(fname) ) return;

    DirList dl( fname );
    BufferString subtimestamp;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString subfnm( dl.fullPath(idx) );
	getTimeLastModified( subfnm, subtimestamp );

	if ( Time::isEarlier( timestamp, subtimestamp ) )
	    timestamp = subtimestamp;
    }
}


BufferString uiObjFileMan::getFileInfo()
{
    BufferString txt;
    if ( !curioobj_ )
	return txt;

    const bool isdir = File::isDirectory( curioobj_->fullUserExpr() );
    mDynamicCastGet(StreamConn*,conn,curioobj_->getConn(Conn::Read))
	if ( !conn && !isdir )
    {
	const BufferString conntyp( curioobj_->connType() );
	if ( conntyp != StreamConn::sType() )
	    txt.add( "From " ).add( conntyp ).add( " data store." );
	else
	    txt = "<empty>";
	txt += "\n\n";
    }
    else
    {
	BufferString fname( isdir ? curioobj_->fullUserExpr()
				  : conn->fileName() );
	FilePath fp( fname );
	int nrfiles = 0;
	const double totsz = getFileSize( fname, nrfiles );

	txt += "Location: "; txt += fp.pathOnly();
	txt += isdir ? "\nDirectory name: " : "\nFile name: ";
	txt += fp.fileName();
	txt += "\nSize on disk: "; txt += getFileSizeString( totsz );
	if ( nrfiles > 1 )
	    { txt += "\nNumber of files: "; txt += nrfiles; }
	BufferString timestr; getTimeStamp( fname, timestr );
	if ( !timestr.isEmpty() ) { txt += "\nLast modified: ";txt += timestr; }
	int txtsz = txt.size()-1;
	if ( txt[ txtsz ] != '\n' ) txt += "\n";
	if ( conn )
	{
	    conn->close();
	    delete conn;
	}
    }

    BufferString crspec;
    curioobj_->pars().get( sKey::CrBy(), crspec );
    if ( crspec.isEmpty() )
	curioobj_->pars().get( "User", crspec );
    if ( !crspec.isEmpty() )
	txt.add( "Created by: " ).add( crspec ).add( "\n" );

    crspec.setEmpty();
    curioobj_->pars().get( sKey::CrAt(), crspec );
    if ( !crspec.isEmpty() )
	txt.add( "Created at: " ).add( crspec ).add( "\n" );

    crspec.setEmpty();
    curioobj_->pars().get( sKey::CrFrom(), crspec );
    if ( !crspec.isEmpty() )
	txt.add( "Created from: " ).add( crspec ).add( "\n" );

    txt.add( "Object ID: " ).add( curioobj_->key() ).add( "\n" );
    return txt;
}


void uiObjFileMan::setInfo( const char* txt )
{
    infofld_->setText( txt );
}


void uiObjFileMan::setPrefWidth( int width )
{
    selgrp_->setPrefWidthInChar( mCast(float,width) );
    infofld_->setPrefWidthInChar( width );
}
