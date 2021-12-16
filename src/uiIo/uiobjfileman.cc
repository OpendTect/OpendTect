/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiobjfileman.h"

#include "uifont.h"
#include "uigeninput.h"
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
			    const IOObjContext& ctxt,
			    const char* ctxtfilter )
    : uiDialog(p,s)
    , ctxt_(*new IOObjContext(ctxt))
    , ctxtfilter_(ctxtfilter)
{
    ctxt_.toselect_.allownonuserselectable_ = true;
    setCtrlStyle( CloseOnly );
    mAttachCB( preFinalise(), uiObjFileMan::finaliseStartCB );
    mAttachCB( postFinalise(), uiObjFileMan::finaliseDoneCB );
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
    sgsu.withctxtfilter_.add( ctxtfilter_ );

    selgrp_ = new uiIOObjSelGrp( listgrp_, ctxt_, uiStrings::sEmptyString(),
									sgsu );
    mAttachCB( selgrp_->selectionChanged, uiObjFileMan::selChg );
    mAttachCB( selgrp_->itemChosen, uiObjFileMan::selChg );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    auto* refreshbut =
	new uiToolButton( selgrp_->getTopGroup(), "refresh", tr("Refresh"),
			  mCB(this,uiObjFileMan,updateCB) );
    refreshbut->attach( rightBorder, selgrp_->getFilterFieldAttachObj() );

    extrabutgrp_ = new uiButtonGroup( listgrp_, "Extra Buttons",
				      OD::Horizontal );
    extrabutgrp_->attach( alignedBelow, selgrp_ );
    extrabutgrp_->attach( ensureBelow, selgrp_ );
    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    extrabutgrp_->setPrefHeight( ft.height()*2 );

    infogrp_ = new uiGroup( this, "Info Group" );
    auto* infolbl = new uiLabel( infogrp_, uiString::emptyString() );
    infolbl->setIcon( "info" );
    infolbl->setToolTip( tr("Data Information") );

    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->attach( rightTo, infolbl );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    auto* dummytb = new uiToolButton( infogrp_, "empty",
					uiString::emptyString(), CallBack() );
    dummytb->attach( rightTo, infofld_ );
    dummytb->display( false );

    auto* notesgrp = new uiGroup( this, "Notes Group" );
    auto* noteslbl = new uiLabel( notesgrp, uiString::emptyString() );
    noteslbl->setIcon( "notes" );
    noteslbl->setToolTip( tr("Notes for selected data") );

    notesfld_ = new uiTextEdit( notesgrp, "User info" );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    notesfld_->setToolTip( tr("Notes") );
    notesfld_->attach( rightTo, noteslbl );
    auto* savebut = new uiToolButton( notesgrp, "save", tr("Save Notes"),
				      mCB(this,uiObjFileMan,saveNotes) );
    savebut->attach( rightTo, notesfld_ );

    setPrefWidth( cPrefWidth );

    auto* sep = new uiSplitter( this, "List-Info splitter", false );
    sep->addGroup( listgrp_ );
    sep->addGroup( infogrp_ );
    sep->addGroup( notesgrp );
}


void uiObjFileMan::finaliseStartCB( CallBacker* )
{
    const bool hasbuttons = extrabutgrp_->nrButtons() > 0;
    extrabutgrp_->display( hasbuttons, !hasbuttons );
    selgrp_->setCurrent( 0 );
}


void uiObjFileMan::finaliseDoneCB( CallBacker* )
{
    initDlg();
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
    if ( !fp.isAbsolute() || fp.isURI() )
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
    uiString msg;
    if ( curioobj_ )
	System::getFreeMBOnDiskMsg( System::getFreeMBOnDisk(*curioobj_), msg );
    toStatusBar( msg );
}


od_int64 uiObjFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    nrfiles = 0;
    BufferString actualfilenm = File::isLink(filenm) ? File::linkTarget(filenm)
						     : filenm;
    if ( !File::exists(actualfilenm.buf()) )
	return 0;

    // File exists ...
    nrfiles = 1;
    od_int64 ret = File::getKbSize( actualfilenm.buf() );
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
    if ( timestamp.isEmpty() || Time::isEarlier(timestamp,ftimestamp) )
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
	const BufferString usrnm = iostrm->fileSpec().dispName();
	if ( iostrm->isMulti() )
	    nrfiles = iostrm->nrFiles();

	if ( !txt.isEmpty() )
	    txt.addNewLine();

	const od_int64 totsz = getFileSize( fname, nrfiles );
	const BufferString fileszstr( File::getFileSizeString( totsz ) );
	if ( isdir )
	{
	    txt.add( "Folder name: " ).add( usrnm );
	    txt.add( "\nTotal size on disk: " ).add( fileszstr );
	    txt.add( "\nNumber of files: " ).add( nrfiles );
	}
	else
	{
	    FilePath fp( usrnm );
	    txt.add( "File name: " ).add( fp.fileName() );
	    fp.set( fname );
	    txt.add( "\nLocation: " ).add( fp.pathOnly() );
	    txt.add( "\nSize: " ).add( fileszstr );
	}
	BufferString timestr; getTimeStamp( fname, timestr );
	if ( !timestr.isEmpty() )
	    txt.add( "\nLast modified: " ).add( timestr );
    }
    txt.addNewLine();

    BufferString crspec;
    curioobj_->pars().get( sKey::CrBy(), crspec );
    if ( crspec.isEmpty() )
	curioobj_->pars().get( "User", crspec );
    if ( !crspec.isEmpty() )
	txt.add( "\nCreated by: " ).add( crspec );

    crspec.setEmpty();
    curioobj_->pars().get( sKey::CrAt(), crspec );
    if ( !crspec.isEmpty() )
	txt.add( "\nCreated at: " )
	   .add( Time::getLocalDateTimeFromString(crspec) );

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


void uiObjFileMan::updateCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiObjFileMan::updateCB );

    const MultiID curmid = selgrp_->currentID();
    selgrp_->fullUpdate( curmid );
    updateList();
}
