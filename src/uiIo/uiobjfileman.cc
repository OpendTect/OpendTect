/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiobjfileman.h"

#include "uifont.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "iox.h"
#include "keystrs.h"
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
    mAttachCB( preFinalize(), uiObjFileMan::finalizeStartCB );
    mAttachCB( postFinalize(), uiObjFileMan::finalizeDoneCB );
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

    selgrp_ = new uiIOObjSelGrp( listgrp_, ctxt_, uiString::empty(), sgsu );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );
    mAttachCB( selgrp_->itemInitRead, uiObjFileMan::initObjRead );
    mAttachCB( selgrp_->launchLocate, uiObjFileMan::doLocateCB );

    auto* refreshbut =
	new uiToolButton( selgrp_->getListField(), "refresh", tr("Refresh"),
			  mCB(this,uiObjFileMan,updateCB) );
    refreshbut->attach( rightOf, selgrp_->getFilterFieldAttachObj() );

    extrabutgrp_ = new uiButtonGroup( listgrp_, "Extra Buttons",
				      OD::Horizontal );
    extrabutgrp_->attach( alignedBelow, selgrp_ );
    extrabutgrp_->attach( ensureBelow, selgrp_ );
    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    extrabutgrp_->setPrefHeight( ft.height()*2 );

    infogrp_ = new uiGroup( this, "Info Group" );

    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->setIcon( "info", tr("Data Information") );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    auto* dummytb = new uiToolButton( infogrp_, "empty",
					uiString::emptyString(), CallBack() );
    dummytb->attach( rightTo, infofld_ );
    dummytb->display( false );

    auto* notesgrp = new uiGroup( this, "Notes Group" );

    notesfld_ = new uiTextEdit( notesgrp, "User info" );
    notesfld_->setIcon( "notes", tr("Notes for selected data") );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    notesfld_->setToolTip( tr("Notes") );
    auto* savebut = new uiToolButton( notesgrp, "save", tr("Save Notes"),
				      mCB(this,uiObjFileMan,saveNotes) );
    savebut->attach( rightTo, notesfld_ );

    setPrefWidth( cPrefWidth );

    auto* sep = new uiSplitter( this, "List-Info splitter", false );
    sep->addGroup( listgrp_ );
    sep->addGroup( infogrp_ );
    sep->addGroup( notesgrp );
}


void uiObjFileMan::finalizeStartCB( CallBacker* )
{
    const bool hasbuttons = extrabutgrp_->nrButtons() > 0;
    extrabutgrp_->display( hasbuttons, !hasbuttons );
    selgrp_->setCurrent( 0 );
}


void uiObjFileMan::finalizeDoneCB( CallBacker* )
{
    uiUserShowWait usw( parent(), tr("Launching manager") );
    mAttachCB( selgrp_->selectionChanged, uiObjFileMan::selChg );
    mAttachCB( selgrp_->itemChosen, uiObjFileMan::selChg );
    mAttachCB( selgrp_->listUpdated, uiObjFileMan::listUpdatedCB );
    initDlg();
    selChg( nullptr );
    checkAllEntriesOK();
}


uiToolButton* uiObjFileMan::addExtraButton( const char* iconnm,
					const uiString& tooltip,
					const CallBack& cb )
{
    return new uiToolButton( extrabutgrp_, iconnm, tooltip, cb );
}


uiToolButton* uiObjFileMan::addManipButton( const char* iconnm,
					const uiString& tooltip,
					const CallBack& cb )
{
    return selgrp_->getManipGroup()->addButton( iconnm, tooltip, cb );
}


void uiObjFileMan::getChosen( BufferStringSet& nms ) const
{
    selgrp_->getChosen( nms );
}


void uiObjFileMan::getChosen( TypeSet<MultiID>& mids ) const
{
    selgrp_->getChosen( mids );
}


void uiObjFileMan::fullUpdate( const MultiID& key )
{
    selgrp_->fullUpdate( key );
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
	fp.set( IOM().rootDir().fullPath().buf() );
	fp.add( ioobj.dirName() );
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


void uiObjFileMan::selChg( CallBacker* )
{
    saveNotes( nullptr );
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


od_int64 uiObjFileMan::getFileSize( const char* filenm ) const
{
    if ( !File::exists(filenm) )
	return 0;

    if ( File::isFile(filenm) )
	return File::getFileSize( filenm );

    if ( !File::isDirectory(filenm) )
	return 0;

    BufferStringSet filelist;
    File::makeRecursiveFileList( filenm, filelist, true );
    const int nrfiles = filelist.size();
    od_int64 ret = 0;
    for ( int idx=0; idx<nrfiles; idx++ )
	ret += File::getFileSize( filelist.get(idx) );

    return ret;
}


int uiObjFileMan::getNrFiles( const char* filenm ) const
{
    int nrfiles = 0;
    if ( !File::exists(filenm) )
	return nrfiles;

    if ( File::isFile(filenm) || !File::isDirectory(filenm) )
	return 1;

    BufferStringSet filelist;
    File::makeRecursiveFileList( filenm, filelist, true );
    return filelist.size();
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
    const StringView ftimestamp = File::timeLastModified( fname );
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
    if ( !isstrm )
    {
	mDynamicCastGet(IOX*,iox,curioobj_)
	if ( iox )
	    txt.add( "Data source: " ).add( iox->ownKey() );
    }
    else
    {
	if ( !curioobj_->implExists(true) )
	    txt.add( "No data exists for " ).add( curioobj_->name() );
	else
	{
	    getBasicFileInfo( txt );
	    BufferString timestr;
	    getTimeStamp( curioobj_->fullUserExpr(), timestr );
	    if ( !timestr.isEmpty() )
		txt.add( "\nLast modified: " ).add( timestr );
	}
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


void uiObjFileMan::getBasicFileInfo( BufferString& txt ) const
{
    const IOStream* iostrm = getIOStream( *curioobj_ );
    if ( !iostrm || iostrm->isBad() )
	return;

    const BufferString fname = curioobj_->fullUserExpr();
    const bool isdir = iostrm && File::isDirectory( fname );
    if ( !txt.isEmpty() )
	txt.addNewLine();

    if ( isdir )
    {
	getBasicDirInfo( *iostrm, txt );
	return;
    }

    const BufferString usrnm = iostrm->fileSpec().dispName();
    FilePath fp( usrnm );
    txt.add( "File name: " ).add( fp.fileName() );
    fp.set( fname );
    txt.add( "\nLocation: " ).add( fp.pathOnly() );
    BufferString fileszstr;
    getFileSizeString( *iostrm, fileszstr );
    txt.add( "\nSize: " ).add( fileszstr );
}


void uiObjFileMan::getBasicDirInfo( const IOStream& iostrm,
				    BufferString& txt ) const
{
    const BufferString usrnm = iostrm.fileSpec().dispName();
    txt.add( "Folder name: " ).add( usrnm );
    BufferString fileszstr;
    getFileSizeString( iostrm, fileszstr );
    txt.add( "\nTotal size on disk: " ).add( fileszstr );
    const int nrfiles = getNrFiles( iostrm.fullUserExpr() );
    txt.add( "\nNumber of files: " ).add( nrfiles );
}


void uiObjFileMan::getFileSizeString( const IOStream& iostrm,
				      BufferString& fileszstr ) const
{
    const od_int64 totsz = getFileSize( iostrm.fullUserExpr() );
    fileszstr = File::getFileSizeString( totsz );
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

    uiUserShowWait usw( this, tr("Reloading the list of Seismic data") );
    const MultiID curmid = selgrp_->currentID();
    NotifyStopper ns( selgrp_->listUpdated, this );
    selgrp_->fullUpdate( curmid );
    updateList();
    checkAllEntriesOK();
}


void uiObjFileMan::listUpdatedCB( CallBacker* )
{
    checkAllEntriesOK();
}
