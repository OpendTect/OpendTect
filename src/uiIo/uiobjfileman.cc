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

#include "ioobjctxt.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "iostrm.h"
#include "od_iostream.h"
#include "webstreamsource.h"
#include "systeminfo.h"

static const int cPrefHeight = 10;
static const int cPrefWidth = 75;

uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    const IOObjContext& ctxt )
    : uiDialog(p,s)
    , curioobj_(0)
    , ctxt_(*new IOObjContext(ctxt))
    , curimplexists_(false)
{
    ctxt_.toselect_.allownonuserselectable_ = true;
    setCtrlStyle( CloseOnly );
    preFinalise().notify( mCB(this,uiObjFileMan,finaliseStartCB) );
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
    uiIOObjSelGrp::Setup sgsu( multisel ? OD::ChooseAtLeastOne
					: OD::ChooseOnlyOne );
    sgsu.allowreloc( withreloc ).allowremove( withrm ).allowsetdefault( true );
    selgrp_ = new uiIOObjSelGrp( listgrp_, ctxt_, uiStrings::sEmptyString(),
									sgsu );
    selgrp_->selectionChanged.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->itemChosen.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    uiToolButton* refreshbut =
	new uiToolButton( selgrp_->getListField(), "refresh", tr("Refresh"),
			  mCB(this,uiObjFileMan,updateCB) );
    refreshbut->attach( rightTo, selgrp_->getFilterField() );

    extrabutgrp_ = new uiButtonGroup( listgrp_, "Extra Buttons",
				      OD::Horizontal );
    extrabutgrp_->attach( alignedBelow, selgrp_ );
    extrabutgrp_->attach( ensureBelow, selgrp_ );
    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    extrabutgrp_->setPrefHeight( ft.height()*2 );

    infogrp_ = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    uiToolButton* dummytb = new uiToolButton( infogrp_, "empty",
					uiString::emptyString(), CallBack() );
    dummytb->attach( rightTo, infofld_ );
    dummytb->display( false );

    uiGroup* notesgrp = new uiGroup( this, "Notes Group" );
    uiLabel* noteslbl = new uiLabel( notesgrp, tr("Notes:") );
    notesfld_ = new uiTextEdit( notesgrp, "User info" );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    notesfld_->setToolTip( tr("Notes") );
    notesfld_->attach( alignedBelow, noteslbl );
    uiToolButton* savebut =
		new uiToolButton( notesgrp, "save", tr("Save Notes"),
	    mCB(this,uiObjFileMan,saveNotes) );
    savebut->attach( rightTo, notesfld_ );

    setPrefWidth( cPrefWidth );

    uiSplitter* sep = new uiSplitter( this, "List-Info splitter",
					OD::Horizontal );
    sep->addGroup( listgrp_ );
    sep->addGroup( infogrp_ );
    sep->addGroup( notesgrp );
    mAttachCB( DBM().entryAdded, uiObjFileMan::updateCB );
    mAttachCB( DBM().entryRemoved, uiObjFileMan::updateCB );
}


void uiObjFileMan::finaliseStartCB( CallBacker* )
{
    const bool hasbuttons = extrabutgrp_->nrButtons() > 0;
    extrabutgrp_->display( hasbuttons, !hasbuttons );
    selgrp_->setCurrent( 0 );
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
    File::Path fp( fnm );
    if ( !fp.isAbsolute() )
    {
	fnm.clean( BufferString::NoSpecialChars );
	fp.set( DBM().survDir() ); fp.add( ioobj.dirName() );
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
    curioobj_ = selgrp_->nrChosen() > 0 ? DBM().get(selgrp_->currentID()) : 0;
    curimplexists_ = curioobj_ && curioobj_->implExists(true);

    ownSelChg();
    if ( curioobj_ )
	mkFileInfo();
    else
	setInfo( "" );

    readNotes();
    uiString msg;
    if ( curioobj_ )
	System::getFreeMBOnDiskUiMsg(System::getFreeMBOnDisk(*curioobj_), msg);
    toStatusBar( msg );
}


od_int64 uiObjFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    nrfiles = 0;
    BufferString actualfilenm = File::isLink(filenm) ? File::linkEnd(filenm)
						     : filenm;
    if ( !File::exists(actualfilenm) )
	return 0;

    // File exists ...
    nrfiles = 1;
    od_int64 ret = File::getKbSize( actualfilenm.buf() );
    if ( !File::isDirectory(actualfilenm) )
    {
	File::Path dirnm( actualfilenm );
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
    File::Path fp( fname );
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
	const BufferString usrnm = iostrm->fileSpec().dispName();
	if ( iostrm->isMulti() )
	    nrfiles = iostrm->nrFiles();

	const od_int64 totsz = getFileSize( fname, nrfiles );
	const BufferString fileszstr( File::getFileSizeString( totsz ) );
	if ( WebStreamSource::willHandle(usrnm) )
	    txt.add( "\nURL: " ).add( usrnm );
	else
	{
	    if ( isdir )
	    {
		txt.add( "\nDirectory name: " ).add( usrnm );
		txt.add( "\nTotal size on disk: " ).add( fileszstr );
		txt.add( "\nNumber of files: " ).add( nrfiles );
	    }
	    else
	    {
		File::Path fp( usrnm );
		txt.add( "\nFile name: " ).add( fp.fileName() );
		fp.set( fname );
		txt.add( "\nLocation: " ).add( fp.pathOnly() );
		txt.add( "\nSize: " ).add( fileszstr );
	    }
	    BufferString timestr; getTimeStamp( fname, timestr );
	    if ( !timestr.isEmpty() )
		txt.add( "\nLast modified: " ).add( timestr );
	}
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


void uiObjFileMan::updateCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiObjFileMan::updateCB );

    const DBKey curmid = selgrp_->currentID();
    selgrp_->fullUpdate( curmid );
}
