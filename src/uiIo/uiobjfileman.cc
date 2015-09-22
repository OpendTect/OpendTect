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
#include "uiioobjselgrp.h"
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
#include "iostrm.h"
#include "od_iostream.h"
#include "systeminfo.h"

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
    ctxt_.toselect.allownonuserselectable_ = true;
    setCtrlStyle( CloseOnly );
}


uiObjFileMan::~uiObjFileMan()
{
    detachAllNotifiers();
    delete curioobj_;
    delete &ctxt_;
}


void uiObjFileMan::createDefaultUI( bool withreloc, bool withrm, bool multisel )
{
    listgrp_ = new uiGroup( this, "List Group" );
    IOM().to( ctxt_.getSelKey(), true );
    uiIOObjSelGrp::Setup sgsu( multisel ? OD::ChooseAtLeastOne
					: OD::ChooseOnlyOne );
    sgsu.allowreloc( withreloc ).allowremove( withrm ).allowsetdefault( true );
    selgrp_ = new uiIOObjSelGrp( listgrp_, ctxt_, 0, sgsu );
    selgrp_->selectionChanged.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->itemChosen.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    infogrp_ = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    uiToolButton* dummytb = new uiToolButton( infogrp_, "empty",
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
    mAttachCB( IOM().entryAdded, uiObjFileMan::updateAddRemoveCB );
    mAttachCB( IOM().entryRemoved, uiObjFileMan::updateAddRemoveCB );
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


static const IOStream* getIOStream( const IOObj& ioobj )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    return iostrm;
}


static bool isIOStream( const IOObj& ioobj )
{
    return getIOStream( ioobj );
}



static BufferString getNotesFileName( const IOObj& ioobj )
{
    BufferString fnm( ioobj.fullUserExpr() );
    FilePath fp( fnm );
    if ( !fp.isAbsolute() )
    {
	fnm.clean( BufferString::NoSpecialChars );
	fp.set( IOM().rootDir() ); fp.add( ioobj.dirName() );
	fp.add( fnm );
    }
    fp.setExtension( "note" );
    return fp.fullPath();
}


void uiObjFileMan::saveNotes( CallBacker* )
{
    BufferString txt = notesfld_->text();
    if ( !curioobj_ )
	return;

    const BufferString filename = getNotesFileName( *curioobj_ );
    if ( txt.isEmpty() )
    {
	if ( File::exists(filename) )
	    File::remove( filename );

	return;
    }

    od_ostream ostrm( filename );
    if ( !ostrm.isOK() )
	return;

    ostrm << txt << '\n';
}


void uiObjFileMan::readNotes()
{
    if ( !curioobj_ )
    {
	notesfld_->setText( sKey::EmptyString() );
	return;
    }

    od_istream istrm( getNotesFileName(*curioobj_) );
    if ( !istrm.isOK() )
    {
	notesfld_->setText( sKey::EmptyString() );
	return;
    }

    BufferString note;
    istrm.getAll( note );
    notesfld_->setText( note );
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    saveNotes(0);
    delete curioobj_;
    curioobj_ = selgrp_->nrChosen() > 0 ? IOM().get(selgrp_->currentID()) : 0;
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
    nrfiles = 0;
    BufferString actualfilenm = File::isLink(filenm) ? File::linkTarget(filenm)
						     : filenm;
    if ( !File::exists(actualfilenm.buf()) )
	return 0;

    // File exists ...
    nrfiles = 1;
    double ret = (double)File::getKbSize( actualfilenm.buf() );
    if ( !File::isDirectory(actualfilenm) )
    {
	FilePath dirnm( actualfilenm );
	dirnm.setExtension( "" );
	if ( !File::isDirectory(dirnm.fullPath()) )
	    return ret;

	actualfilenm = dirnm.fullPath();
    }

    // It's a directory ...
    BufferStringSet filelist;
    File::makeRecursiveFileList( actualfilenm.buf(), filelist, true );
    nrfiles = filelist.size();
    for ( int idx=0; idx<nrfiles; idx++ )
	ret += File::getKbSize( filelist.get(idx) );

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

    const bool isstrm = isIOStream( *curioobj_ );
    const BufferString fname = curioobj_->fullUserExpr();
    const bool isdir = isstrm && File::isDirectory( fname );
    if ( !isstrm )
	txt.add( "Data source: " ).add( curioobj_->connType() );
    else
    {
	int nrfiles = 0;
	const IOStream* iostrm = getIOStream( *curioobj_ );
	if ( iostrm->isMulti() )
	    nrfiles = iostrm->fileNumbers().nrSteps() + 1;

	const double totsz = getFileSize( fname, nrfiles );
	BufferString fileszstr( getFileSizeString(totsz) );
	if ( isdir )
	{
	    txt.add( "\nDirectory name: " ).add( fname );
	    txt.add( "\nTotal size on disk: " ).add( fileszstr );
	    txt.add( "\nNumber of files: " ).add( nrfiles );
	}
	else
	{
	    FilePath fp( fname );
	    txt.add( "\nFile name: " ).add( fp.fileName() );
	    txt.add( "\nLocation: " ).add( fp.pathOnly() );
	    txt.add( "\nSize: " ).add( fileszstr );
	}
	BufferString timestr; getTimeStamp( fname, timestr );
	if ( !timestr.isEmpty() )
	    txt.add( "\nLast modified: " ).add( timestr );
    }
    txt.add( "\n" );

    BufferString crspec;
    curioobj_->pars().get( sKey::CrBy(), crspec );
    if ( crspec.isEmpty() )
	curioobj_->pars().get( "User", crspec );
    if ( !crspec.isEmpty() )
	txt.add( "\nCreated by: " ).add( crspec );

    crspec.setEmpty();
    curioobj_->pars().get( sKey::CrAt(), crspec );
    if ( !crspec.isEmpty() )
	txt.add( "\nCreated at: " ).add( crspec );

    crspec.setEmpty();
    curioobj_->pars().get( sKey::CrFrom(), crspec );
    if ( !crspec.isEmpty() )
	txt.add( "\nCreated from: " ).add( crspec );

    txt.add( "\nStorage type: " ).add( curioobj_->translator() );
    txt.add( "\nObject ID: " ).add( curioobj_->key() );
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


void uiObjFileMan::updateAddRemoveCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiObjFileMan::updateAddRemoveCB );

    selgrp_->fullUpdate( -1 );
}
