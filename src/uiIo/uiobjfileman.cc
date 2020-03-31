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
#include "keystrs.h"
#include "timefun.h"
#include "dbman.h"
#include "iostrm.h"
#include "od_iostream.h"
#include "systeminfo.h"

static const int cPrefHeight = 10;
static const int cPrefWidth = 75;

uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    const IOObjContext& ctxt, const char* ctxtfilt )
    : uiDialog(p,s)
    , curioobj_(0)
    , ctxt_(*new IOObjContext(ctxt))
    , curimplexists_(false)
    , ctxtfilt_(ctxtfilt)
{
    ctxt_.toselect_.allownonuserselectable_ = true;
    setCtrlStyle( CloseOnly );
    mAttachCB( preFinalise(), uiObjFileMan::finaliseStartCB );
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
    if ( !ctxtfilt_.isEmpty() )
	sgsu.withctxtfilter( ctxtfilt_ );

    selgrp_ = new uiIOObjSelGrp( listgrp_, ctxt_, uiString::empty(), sgsu );
    selgrp_->selectionChanged.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->itemChosen.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp_->getListField()->setHSzPol( uiObject::Medium );

    extrabutgrp_ = new uiButtonGroup( listgrp_, "Extra Buttons",
				      OD::Horizontal );
    extrabutgrp_->attach( alignedBelow, selgrp_ );
    extrabutgrp_->attach( ensureBelow, selgrp_ );
    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    extrabutgrp_->setPrefHeight( ft.height()*2 );

    infogrp_ = new uiGroup( this, "Info Group" );
    uiLabel* infolbl = new uiLabel( infogrp_, uiString::empty() );
    infolbl->setIcon( "info" );
    infolbl->setToolTip( tr("Data Information") );

    infofld_ = new uiTextEdit( infogrp_, "Object Info", true );
    infofld_->attach( rightTo, infolbl );
    infofld_->setPrefHeightInChar( cPrefHeight );
    infofld_->setStretch( 2, 2 );
    uiToolButton* dummytb = new uiToolButton( infogrp_, "empty",
					uiString::empty(), CallBack() );
    dummytb->attach( rightTo, infofld_ );
    dummytb->display( false );

    uiGroup* notesgrp = new uiGroup( this, "Notes Group" );
    uiLabel* noteslbl = new uiLabel( notesgrp, uiString::empty() );
    noteslbl->setIcon( "notes" );
    noteslbl->setToolTip( tr("Notes for selected data") );

    notesfld_ = new uiTextEdit( notesgrp, "User info" );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    notesfld_->setToolTip( tr("Notes") );
    notesfld_->attach( rightTo, noteslbl );
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
    BufferString fnm( ioobj.mainFileName() );
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
	notesfld_->setText( OD::EmptyString() );
	return;
    }

    od_istream istrm( getNotesFileName(*curioobj_) );
    if ( !istrm.isOK() )
    {
	notesfld_->setText( OD::EmptyString() );
	return;
    }

    BufferString note;
    istrm.getAll( note );
    notesfld_->setText( note );
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    saveNotes(0);
    updateFromSelected();
    BufferString typestr;
    if ( curioobj_ )
	typestr = curioobj_->pars().find(sKey::Type());
    if ( !typestr.isEmpty() )
	selgrp_->setSurveyDefaultSubsel(
				IOPar::compKey(BufferString::empty(),typestr));
}

void uiObjFileMan::updateFromSelected()
{
    delete curioobj_;
    curioobj_ = selgrp_->nrChosen() > 0 ? selgrp_->currentID().getIOObj() : 0;
    curimplexists_ = curioobj_ && curioobj_->implExists(true);

    ownSelChg();
    refreshItemInfo();
    readNotes();

    uiString msg;
    if ( curioobj_ )
	System::getFreeMBOnDiskUiMsg(System::getFreeMBOnDisk(*curioobj_), msg);
    toStatusBar( msg );
}


void uiObjFileMan::refreshItemInfo()
{
    if ( !curioobj_ )
	setInfo( uiString::empty() );
    else
    {
	uiPhraseSet info;
	getItemInfo( *curioobj_, info );
	uiString allinfo( info.cat(uiString::NoSep) );
	info.setEmpty();
	getFileInfo( *curioobj_, info  );
	allinfo.appendPhrase( info.cat(uiString::NoSep), uiString::NoSep,
			      uiString::AfterEmptyLine );
	setInfo( allinfo );
    }
}


uiString& uiObjFileMan::addObjInfo( uiPhraseSet& inf, const uiWord& subj,
					    const uiString& val ) const
{
    inf.addKeyValue( subj, val );
    return inf.get( inf.size()-1 );
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
	dirnm.setExtension( 0 );
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
				 BufferString& timestamp ) const
{
    timestamp = File::timeLastModified( fname );
    File::Path fp( fname );
    fp.setExtension( 0 );
    const BufferString dirnm = fp.fullPath();
    if ( File::isDirectory(dirnm) )
	getTimeLastModified( dirnm, timestamp );
}


void uiObjFileMan::getTimeLastModified( const char* fname,
					BufferString& timestamp ) const
{
    const FixedString ftimestamp = File::timeLastModified( fname );
    if ( timestamp.isEmpty() || Time::isEarlierStamp(timestamp,ftimestamp) )
	timestamp = ftimestamp;

    if ( !File::isDirectory(fname) )
	return;

    DirList dl( fname );
    BufferString subtimestamp;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString subfnm( dl.fullPath(idx) );
	getTimeLastModified( subfnm, subtimestamp );

	if ( Time::isEarlierStamp( timestamp, subtimestamp ) )
	    timestamp = subtimestamp;
    }
}


bool uiObjFileMan::getFileInfo( const IOObj& ioobj, uiPhraseSet& info ) const
{
    const bool isstrm = isIOStream( ioobj );
    const BufferString fname = ioobj.mainFileName();
    const bool isdir = isstrm && File::isDirectory( fname );
    if ( !isstrm )
	addObjInfo( info, tr("Data source"), ioobj.connType() );
    else
    {
	int nrfiles = 0;
	const IOStream* iostrm = getIOStream( ioobj );
	const BufferString usrnm = iostrm->fileSpec().dispName();
	if ( iostrm->isMulti() )
	    nrfiles = iostrm->nrFiles();

	const od_int64 totsz = getFileSize( fname, nrfiles );
	const BufferString fileszstr( File::getFileSizeString( totsz ) );
	if ( isdir )
	{
	    addObjInfo( info, tr("Directory name"), usrnm );
	    addObjInfo( info, tr("Total size on disk"), fileszstr );
	    addObjInfo( info, tr("Number of files"), nrfiles );
	}
	else
	{
	    File::Path fp( usrnm );
	    addObjInfo( info, uiStrings::sFileName(), fp.fileName() );
	    fp.set( fname );
	    addObjInfo( info, uiStrings::sLocation(), fp.pathOnly() );
	    addObjInfo( info, uiStrings::sSize(), fileszstr );
	}
	BufferString timestr; getTimeStamp( fname, timestr );
	if ( !timestr.isEmpty() )
	    addObjInfo( info, tr("Last modified"),
		   Time::getUsrDateTimeStringFromISOUTC(timestr) );
    }

    BufferString str;
#   define mAddInfFromPars(ky,fn) \
    str.setEmpty(); ioobj.pars().get( sKey::ky(), str ); \
    if ( !str.isEmpty() ) \
	    addObjInfo( info, uiStrings::s##ky(), str )
    mAddInfFromPars( CrBy, toString );
    mAddInfFromPars( CrAt, Time::getUsrDateTimeStringFromISOUTC );
    mAddInfFromPars( CrFrom, toString );

    addObjInfo( info, uiStrings::sStorageType(), ioobj.translator() );
    addObjInfo( info, uiStrings::sObjectID(), ioobj.key() );

    return true;
}


void uiObjFileMan::setInfo( const uiString& txt )
{
    infofld_->setText( txt );
}


void uiObjFileMan::setPrefWidth( int width )
{
    selgrp_->setPrefWidthInChar( mCast(float,width) );
    infofld_->setPrefWidthInChar( width );
}
