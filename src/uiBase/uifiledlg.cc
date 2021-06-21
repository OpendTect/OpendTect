/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/09/2000
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "q_uiimpl.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "fileformat.h"
#include "oddirs.h"
#include "separstr.h"

#include "uiparentbody.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistrings.h"

#include <QFileDialog>
#include <QPushButton>

mUseQtnamespace

const char* uiFileDialog::filesep_ = ";";

class ODFileDialog : public QFileDialog
{
public:

ODFileDialog( const QString& dirname, const QString& fltr, QWidget* p,
		const char* caption )
    : QFileDialog(p,caption,dirname,fltr)
{ setModal( true ); }

};


static QFileDialog::FileMode qmodeForUiMode( OD::FileSelectionMode mode )
{
    switch( mode )
    {
    case OD::SelectFileForRead		: return QFileDialog::ExistingFile;
    case OD::SelectDirectory		: return QFileDialog::Directory;
    case OD::SelectMultiFile		: return QFileDialog::ExistingFiles;
    default				: return QFileDialog::AnyFile;
    }
}

#define mCommon \
    fname_ = fname; \
    caption_ = caption; \
    parnt_ = parnt; \
    confirmoverwrite_ = true; \
\
    if ( caption.isEmpty() ) \
	setDefaultCaption(); \

mStartAllowDeprecatedSection

uiFileDialog::uiFileDialog( uiParent* parnt, bool forread, const char* fname,
			    const char* fltr, uiString caption )
	: mode_(forread ? OD::SelectFileForRead : OD::SelectFileForWrite)
        , forread_( forread )
	, filter_( fltr )
	, addallexts_(forread)
{  mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, Mode md,
			    const char* fname, const char* fltr,
			    uiString caption )
	: mode_(md)
        , forread_(true)
	, filter_(fltr)
	, addallexts_(true)
{ mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, Type typ,
			    const char* fname, uiString caption )
	: mode_(OD::SelectFileForWrite)
        , forread_(true)
	, addallexts_(true)
{
    mCommon

    switch ( typ )
    {
	case OD::ImageContent:
	    filter_ = File::Format::imageFiles().getFileFilter(); break;
	case OD::TextContent:
	    filter_ = File::Format::textFiles().getFileFilter(); break;
	case OD::HtmlContent:
	    filter_ = "Html (*.htm *.html)"; break;
	default: break;
    }
}

mStopAllowDeprecatedSection


int uiFileDialog::go()
{
    const File::Path fp( fname_ );
    fname_ = fp.fullPath();
    BufferString dirname;
    if ( File::isDirectory(fname_) )
    {
	dirname = fname_;
	fname_.setEmpty();
    }
    else
    {
	if ( !File::isDirectory(fp.pathOnly()) )
	{
	    dirname = GetPersonalDir();
	    fname_.setEmpty();
	}
	else if ( !File::exists(fname_) &&
		  (mode_ == OD::SelectFileForWrite
		    || mode_ == OD::SelectMultiFile) )
	{
	    dirname = fp.pathOnly();
	    fname_.setEmpty();
	}
	else
	{
	    dirname = fp.pathOnly();
	    fname_ = fp.fileName();
	}
    }

    if ( GetEnvVarYN("OD_FILE_SELECTOR_BROKEN") )
    {
	uiDialog dlg( parnt_, uiDialog::Setup(tr("Specify file name"),
			    tr("System file selection unavailable!"),
                            mNoHelpKey) );
	uiLineEdit* le = new uiLineEdit( &dlg, "File name" );
	le->setText( dirname );
	new uiLabel( &dlg, tr("File name"), le );
	if ( !dlg.go() ) return 0;
	fn = le->text(); filenames_.add( fn );
	return 1;
    }

    QWidget* qparent = 0;
    if ( parnt_ && parnt_->pbody() )
	qparent = parnt_->pbody()->managewidg();

    BufferString flt( filter_ );
    if ( !flt.isEmpty() )
    {
	if ( addallexts_ )
	{
	    flt += ";;All files (*";
#ifdef __win__
	    flt += " *.*";
#endif
	    flt += ")";
	}
    }

    BufferString addendum;
    const uiString wintitle =
	uiMainWin::uniqueWinTitle( caption_, 0, &addendum );
    const BufferString utfwintitle( toString(caption_), addendum );
    int refnr = beginCmdRecEvent( utfwintitle.buf() );
    PtrMan<ODFileDialog> fd = new ODFileDialog( QString(dirname), QString(flt),
					 qparent, "File dialog" );
    fd->selectFile( QString(fname_) );
    fd->setAcceptMode( forread_ ? QFileDialog::AcceptOpen
				: QFileDialog::AcceptSave );
    const QFileDialog::FileMode qfmode = qmodeForUiMode( mode_ );
    fd->setFileMode( qfmode );
    if ( qfmode == QFileDialog::Directory )
	fd->setOption( QFileDialog::ShowDirsOnly );

    fd->setWindowTitle( toQString(wintitle) );
    fd->setOption( QFileDialog::DontConfirmOverwrite, !confirmoverwrite_ );
    if ( !currentdir_.isEmpty() )
	fd->setDirectory( QString(currentdir_.buf()) );
    if ( selectedfilter_.size() )
	fd->selectNameFilter( QString(selectedfilter_) );
    if ( !defaultextension_.isEmpty() )
	fd->setDefaultSuffix( QString(defaultextension_.buf()) );

#ifdef __win__
    fd->setViewMode( QFileDialog::Detail );
#endif

    QList<QPushButton*> qpblst = fd->findChildren<QPushButton*>("");
    foreach(QPushButton* qpb,qpblst)
    {
	if ( qpb->text() == toQString(uiStrings::sSave()) ||
	     qpb->text() == toQString(uiStrings::sOpen()) ||
	     qpb->text() == "Choose" )
	     qpb->setText( "OK" );
    }

    if ( fd->exec() != QDialog::Accepted )
    {
	int res = processExternalFilenames( dirname, flt );
	endCmdRecEvent( refnr, res );
	return res;
    }

    QStringList selfiles = fd->selectedFiles();
    if ( !selfiles.isEmpty() )
	fn = selfiles[0];

    selectedfilter_ = fd->selectedNameFilter();

#ifdef __win__
    fn.replace( '/', '\\' );
#endif

    for ( int idx=0; idx<selfiles.size(); idx++ )
    {
	BufferString bs( selfiles[idx] );
#ifdef __win__
	bs.replace( '/', '\\' );
#endif
	filenames_.add( bs );
    }

    endCmdRecEvent( refnr, true );
    return 1;
}


void uiFileDialog::getFileNames( BufferStringSet& fnms ) const
{
    fnms = filenames_;
}


void uiFileDialog::joinFileNamesIntoSingleString( const BufferStringSet& list,
				BufferString& string )
{
    QStringList qlist;
    for ( int idx=0; idx<list.size(); idx++ )
	qlist.append( (QString)list[idx]->buf() );

    string = qlist.join( (QString)filesep_ );
}


void uiFileDialog::separateFileNamesFromSingleString(
			const BufferString& string, BufferStringSet& list )
{
    QString qstr( string.buf() );
    QStringList qlist = qstr.split( (QString)filesep_ );
    for ( int idx=0; idx<qlist.size(); idx++ )
	list.add( qlist[idx] );
}


FileMultiString* uiFileDialog::externalfilenames_ = 0;
BufferString uiFileDialog::extfilenameserrmsg_ = "";


void uiFileDialog::setExternalFilenames( const FileMultiString& fms )
{
    extfilenameserrmsg_.setEmpty();

    if ( !externalfilenames_ )
	externalfilenames_ = new FileMultiString( fms );
    else
	*externalfilenames_ = fms;
}


const char* uiFileDialog::getExternalFilenamesErrMsg()
{
    return extfilenameserrmsg_.isEmpty() ? 0 : extfilenameserrmsg_.buf();
}


static bool filterIncludesExt( const char* fltr, const char* ext )
{
    if ( !fltr )
	return false;

    const char* fltptr = fltr;
    while ( *fltptr != '\0' )
    {
	fltptr++;
	if ( *(fltptr-1) != '*' )
	    continue;
	if ( *(fltptr) != '.' )
	    return true;

	if ( !ext )
	    continue;
	const char* extptr = ext;
	while ( true )
	{
	    fltptr++;
	    if ( *extptr == '\0' )
	    {
		if ( !iswalnum(*fltptr) )
		    return true;
		break;
	    }
	    if ( tolower(*extptr) != tolower(*fltptr) )
		break;
	    extptr++;
	}
    }
    return false;
}


#define mRetMsg( pathname, msg, msgretval ) \
{ \
    if ( msg ) \
    { \
	extfilenameserrmsg_ += msgretval ? "!" : ""; \
	extfilenameserrmsg_ += "Path name \""; \
	extfilenameserrmsg_ += pathname; \
	extfilenameserrmsg_ += "\" "; \
	extfilenameserrmsg_ += msg; \
    } \
    delete externalfilenames_; \
    externalfilenames_ = 0; \
    return msg ? msgretval : 1; \
}

#define mRetErrMsg( pathname, msg ) \
    mRetMsg( pathname, msg, 0 )


int uiFileDialog::processExternalFilenames( const char* dir,
					    const char* filters )
{
    if ( !externalfilenames_ )
	return 0;

    filenames_.setEmpty();
    fn.setEmpty();

    if ( externalfilenames_->isEmpty() )
	mRetErrMsg( "", mode_==OD::SelectMultiFile ? 0
				: "should not be empty" );

    if ( !dir )
	dir = currentdir_.isEmpty() ? GetPersonalDir() : currentdir_.buf();
    if ( !filters )
	filters = filter_.buf();

    TypeSet<int> fltidxset;
    BufferStringSet filterset;
    const SeparString fltsep( filters, uiFileDialog::filesep_[0] );
    for ( int fltidx=0; fltidx<fltsep.size(); fltidx++ )
    {
	filterset.add( fltsep[fltidx] );
	fltidxset += fltidx;
    }

    int wrongextidx = -1;
    for ( int idx=0; idx<externalfilenames_->size(); idx++ )
    {
	BufferString fname( (*externalfilenames_)[idx] );
	File::Path fp( fname );
	if ( !fp.isAbsolute() )
	    fp = File::Path( dir, fname );
	fname = fp.fullPath();

	if ( !idx && externalfilenames_->size()>1
		&& mode_!=OD::SelectMultiFile )
	    mRetErrMsg( fname, "expected to be solitary" );

	if ( File::isDirectory(fname) )
	{
	    if ( isFile(mode_) )
		mRetErrMsg( fname, "specifies an existing directory" );
	    if ( !forread_ && !File::isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only directory" );
	}
	else
	{
	    if ( isDirectory(mode_) )
		mRetErrMsg( fname, "specifies no existing directory" );

	    if ( !File::exists(fname) )
	    {
		if ( mode_ != OD::SelectFileForWrite )
		    mRetErrMsg( fname, "specifies no existing file" );
		if ( fp.nrLevels() > 1 )
		{
		    if ( !File::isDirectory(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in non-existing directory" );
		    if ( !forread_ && !File::isWritable(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in a read-only directory" );
		}
	    }
	    else if ( !forread_ && !File::isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only file" );
	}

	BufferString* bs = new BufferString( fname );
#ifdef __win__
	bs->replace( '/', '\\' );
#endif
	filenames_ += bs;

	if ( !idx )
	    fn = *bs;

	if ( wrongextidx < 0 )
	    wrongextidx = idx;

	for ( int fltidx=0; fltidx<filterset.size(); fltidx++ )
	{
	    if ( !filterIncludesExt(filterset[fltidx]->buf(),fp.extension()) )
		fltidxset -= fltidx;
	    else if ( wrongextidx == idx )
		wrongextidx = -1;
	}
    }

    if ( !fltidxset.isEmpty() )
	selectedfilter_ = filterset[fltidxset[0]]->buf();

    if ( *filters && wrongextidx>=0 )
    {
	mRetMsg( (*externalfilenames_)[wrongextidx],
		 "has an incompatible file extension", 1 );
    }

    mRetErrMsg( "", 0 );
}


int uiFileDialog::beginCmdRecEvent( const char* wintitle )
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QFileDlg " );
    msg += wintitle;

#ifdef __lux32__
    return carrier->beginCmdRecEvent( (od_uint32)this, msg );
#else
    return carrier->beginCmdRecEvent( (od_int64)this, msg );
#endif

}


void uiFileDialog::endCmdRecEvent( int refnr, bool ok )
{
    BufferString msg( "QFileDlg" );
    if ( ok && !filenames_.isEmpty() )
    {
	FileMultiString fms;
	fms += filenames_;
	msg += " "; msg += fms;
    }

    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( carrier )
#ifdef __lux32__
	carrier->endCmdRecEvent( (od_uint32) this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_int64)this, refnr, msg );
#endif

}


void uiFileDialog::setDefaultCaption()
{
    caption_ = isDirectory(mode_) ? tr("Select Directory") : tr("Select File");
}


void uiFileDialog::setDefaultExtension( const char* ext )
{
    defaultextension_ = ext;
}


const char* uiFileDialog::getDefaultExtension() const
{
    return defaultextension_.buf();
}
