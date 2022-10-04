/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "q_uiimpl.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
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

static bool sUseNativeDialog()
{
    mDefineStaticLocalObject( int, native, = -1 );
    if ( native != -1 )
	return bool(native);

    native = 1;
#ifdef __lux__
    const BufferString xdgsessiondesktop = GetEnvVar( "XDG_SESSION_DESKTOP" );
    const BufferString xdgcurrentdesktop = GetEnvVar( "XDG_CURRENT_DESKTOP" );
    if ( xdgsessiondesktop.isEqual("gnome",OD::CaseInsensitive) ||
	 xdgcurrentdesktop.isEqual("gnome",OD::CaseInsensitive) )
	native = 0;
#endif
    return bool(native);
}


class ODFileDialog : public QFileDialog
{
public:

ODFileDialog( const QString& dirname, const QString& fltr, QWidget* p,
	      const char* caption )
    : QFileDialog(p,caption,dirname,fltr)
{
    setModal( true );
    setOption( QFileDialog::DontUseNativeDialog, !sUseNativeDialog() );
}

};


static QFileDialog::FileMode qmodeForUiMode( uiFileDialog::Mode mode )
{
    switch( mode )
    {
    case uiFileDialog::ExistingFile	: return QFileDialog::ExistingFile;
    case uiFileDialog::Directory	: return QFileDialog::Directory;
    case uiFileDialog::DirectoryOnly	: return QFileDialog::Directory;
    case uiFileDialog::ExistingFiles	: return QFileDialog::ExistingFiles;
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
    { \
	setDefaultCaption(); \
    }


uiFileDialog::uiFileDialog( uiParent* parnt, bool forread,
			    const char* fname, const char* fltr,
			    const uiString& caption )
	: mode_(forread ? ExistingFile : AnyFile)
        , forread_( forread )
	, filter_( fltr )
	, addallexts_(forread)
{  mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, Mode md,
			    const char* fname, const char* fltr,
			    const uiString& caption )
	: mode_(md)
        , forread_(true)
	, filter_(fltr)
	, addallexts_(true)
{ mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, uiFileDialog::Type typ,
			    const char* fname, const uiString& caption )
	: mode_(AnyFile)
        , forread_(true)
	, addallexts_(true)
{
    mCommon

    switch ( typ )
    {
	case Img: filter_ = "JPEG (*.jpg *.jpeg);;PNG (*.png)"; break;
	case Txt: filter_ = "Text (*.txt *.dat)"; break;
	case Html: filter_ = "Html (*.htm *.html)"; break;
	default: break;
    }
}


uiFileDialog::~uiFileDialog()
{}


int uiFileDialog::go()
{
    const FilePath fp( fname_ );
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
		  (mode_ == ExistingFile || mode_ == ExistingFiles) )
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
			tr("System file selection unavailable!"), mNoHelpKey) );
	uiLineEdit* le = new uiLineEdit( &dlg, "File name" );
	le->setText( dirname );
	le->setDefaultTextValidator();

	new uiLabel( &dlg, tr("File name"), le );
	if ( !dlg.go() ) return 0;
	fn = le->text(); filenames_.add( fn );
	return 1;
    }

    QWidget* qparent = 0;
    if ( parnt_ && parnt_->pbody() )
	qparent = parnt_->pbody()->managewidg();

    BufferString flt( filter_ );
    if ( filter_.size() )
    {
	if ( addallexts_ )
	    flt.add( ";;" ).add( File::allFilesFilter() );
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
    {
	fd->setOption( QFileDialog::ShowDirsOnly );
	fd->setAcceptMode( QFileDialog::AcceptOpen );
    }

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
	const QString btxt = qpb->text();
	if ( btxt=="&Save" || btxt=="&Open" || btxt=="&Choose" )
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


void uiFileDialog::list2String( const BufferStringSet& list,
				BufferString& string )
{
    QStringList qlist;
    for ( int idx=0; idx<list.size(); idx++ )
	qlist.append( (QString)list[idx]->buf() );

    string = qlist.join( (QString)filesep_ );
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
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
	mRetErrMsg( "", mode_==ExistingFiles ? 0 : "should not be empty" );

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
	FilePath fp( fname );
	if ( !fp.isAbsolute() )
	    fp = FilePath( dir, fname );
	fname = fp.fullPath();

	if ( !idx && externalfilenames_->size()>1 && mode_!=ExistingFiles )
	    mRetErrMsg( fname, "expected to be solitary" );

	if ( File::isDirectory(fname) )
	{
	    if ( mode_!=Directory && mode_!=DirectoryOnly )
		mRetErrMsg( fname, "specifies an existing folder" );
	    if ( !forread_ && !File::isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only folder" );
	}
	else
	{
	    if ( mode_==Directory || mode_==DirectoryOnly )
		mRetErrMsg( fname, "specifies no existing folder" );

	    if ( !File::exists(fname) )
	    {
		if ( mode_ != AnyFile )
		    mRetErrMsg( fname, "specifies no existing file" );
		if ( fp.nrLevels() > 1 )
		{
		    if ( !File::isDirectory(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in non-existing folder" );
		    if ( !forread_ && !File::isWritable(fp.pathOnly()) )
			mRetErrMsg( fname, "ends in a read-only folder" );
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
    uiMainWin* carrier = uiMain::instance().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QFileDlg " );
    msg += wintitle;

#ifdef __lux32__
    return carrier->beginCmdRecEvent( (od_uint32) this, msg );
#else
    return carrier->beginCmdRecEvent( (od_uint64) this, msg );
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

    uiMainWin* carrier = uiMain::instance().topLevel();
    if ( carrier )
#ifdef __lux32__
	carrier->endCmdRecEvent( (od_uint32) this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_uint64) this, refnr, msg );
#endif

}


void uiFileDialog::setDefaultCaption()
{
    caption_ = (mode_==Directory || mode_==DirectoryOnly)
        ? tr("Directory selection")
        : tr("File selection");
}


void uiFileDialog::setDefaultExtension( const char* ext )
{ defaultextension_ = ext; }


const char* uiFileDialog::getDefaultExtension() const
{ return defaultextension_.buf(); }
