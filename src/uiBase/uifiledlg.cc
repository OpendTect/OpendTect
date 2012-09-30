/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/09/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifiledlg.h"

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

#include <QFileDialog>
#include <QPushButton>

mUseQtnamespace

const char* uiFileDialog::filesep_ = ";";

class ODFileDialog : public QFileDialog
{
public:

ODFileDialog( const QString& dirname, const QString& fltr=QString::null,
	      QWidget* p=0, const char* caption=0, bool modal=false )
    : QFileDialog(p,caption,dirname,fltr)
{ setModal( modal ); }

ODFileDialog( QWidget* p=0, const char* caption=0, bool modal=false )
    : QFileDialog(p,caption)
{ setModal( modal ); }

};


static QFileDialog::FileMode qmodeForUiMode( uiFileDialog::Mode mode )
{
    switch( mode )
    {
    case uiFileDialog::AnyFile		: return QFileDialog::AnyFile;
    case uiFileDialog::ExistingFile	: return QFileDialog::ExistingFile;
    case uiFileDialog::Directory	: return QFileDialog::Directory;
    case uiFileDialog::DirectoryOnly	: return QFileDialog::DirectoryOnly;
    case uiFileDialog::ExistingFiles	: return QFileDialog::ExistingFiles;
    }

    return QFileDialog::AnyFile;
}

#define mCommon \
    fname_ = fname; \
    caption_ = caption; \
    parnt_ = parnt; \
    confirmoverwrite_ = true; \
\
    if ( !caption || !*caption ) \
    { \
	caption_ = (mode_==Directory || mode_==DirectoryOnly) ? \
		   "Directory selection" : "File selection"; \
    }


uiFileDialog::uiFileDialog( uiParent* parnt, bool forread,
			    const char* fname, const char* fltr,
			    const char* caption )
	: mode_(forread ? ExistingFile : AnyFile)
        , forread_( forread )
	, filter_( fltr )
	, addallexts_(false)
{  mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, Mode md,
			    const char* fname, const char* fltr,
			    const char* caption )
	: mode_(md)
        , forread_(true)
	, filter_(fltr)
	, addallexts_(false)
{ mCommon }


uiFileDialog::uiFileDialog( uiParent* parnt, uiFileDialog::Type typ,
			    const char* fname, const char* caption )
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


int uiFileDialog::go()
{
    FilePath fp( fname_ );
    fname_ = fp.fullPath();
    BufferString dirname;
    if ( !File::isDirectory(fname_) )
    {
	if ( !File::isDirectory(fp.pathOnly()) )
	{
	    dirname = GetPersonalDir();
	    fname_ = "";
	}
	else if ( !File::exists(fname_) &&
		  (mode_ == ExistingFile || mode_ == ExistingFiles) )
	{
	    dirname = fp.pathOnly();
	    fname_ = "";
	}
	else
	{
	    dirname = fp.pathOnly();
	    fname_ = fp.fileName();
	}
    }
    else
    {
	dirname = fname_;
	fname_ = "";
    }

    if ( GetEnvVarYN("OD_FILE_SELECTOR_BROKEN") )
    {
	uiDialog dlg( parnt_, uiDialog::Setup("Specify file name",
			    "System file selection unavailable!",mNoHelpID) );
	uiLineEdit* le = new uiLineEdit( &dlg, "File name" );
	le->setText( dirname );
	new uiLabel( &dlg, "File name", le );
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
	{
	    flt += ";;All files (*";
#ifdef __win__
	    flt += " *.*";
#endif
	    flt += ")";
	}
    }

    const char* wintitle = uiMainWin::uniqueWinTitle( caption_ );
    int refnr = beginCmdRecEvent( wintitle );
    PtrMan<ODFileDialog> fd = new ODFileDialog( QString(dirname), QString(flt),
					 qparent, "File dialog", true );
    fd->selectFile( QString(fname_) );
    fd->setAcceptMode( forread_ ? QFileDialog::AcceptOpen
	    			: QFileDialog::AcceptSave );
    fd->setFileMode( qmodeForUiMode(mode_) );
    fd->setWindowTitle( QString(wintitle) );
    fd->setConfirmOverwrite( confirmoverwrite_ );
    if ( !currentdir_.isEmpty() )
	fd->setDirectory( QString(currentdir_.buf()) );
    if ( selectedfilter_.size() )
	fd->selectFilter( QString(selectedfilter_) );

#ifdef __win__
    fd->setViewMode( QFileDialog::Detail );
#endif

    QList<QPushButton*> qpblst = fd->findChildren<QPushButton*>("");
    foreach(QPushButton* qpb,qpblst)
    {
	if ( qpb->text() == "&Save" || qpb->text() == "&Open"
				    || qpb->text() == "&Choose" )
	    qpb->setText( "&Ok" );
    }

    if ( fd->exec() != QDialog::Accepted )
    {
    	int res = processExternalFilenames( dirname, flt );
	endCmdRecEvent( refnr, res );
	return res;
    }

    QStringList selfiles = fd->selectedFiles();
    if ( !selfiles.isEmpty() )
	fn = mQStringToConstChar( selfiles[0] );

    selectedfilter_ = fd->selectedFilter().toAscii().constData();

#ifdef __win__
    replaceCharacter( fn.buf(), '/', '\\' );
#endif

    for ( int idx=0; idx<selfiles.size(); idx++ )
    {
	BufferString bs( mQStringToConstChar(selfiles[idx]) );
#ifdef __win__
	replaceCharacter( bs.buf(), '/', '\\' );
#endif
	filenames_.add( bs );
    }

    endCmdRecEvent( refnr, true );
    return 1;
}


void uiFileDialog::getFileNames( BufferStringSet& fnms ) const
{
    deepCopy( fnms, filenames_ );
}


void uiFileDialog::list2String( const BufferStringSet& list,
				BufferString& string )
{
    QStringList qlist;
    for ( int idx=0; idx<list.size(); idx++ )
	qlist.append( (QString)list[idx]->buf() );

    string = qlist.join( (QString)filesep_ ).toAscii().constData();
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
{
    QString qstr( string.buf() );
    QStringList qlist = qstr.split( (QString)filesep_ );
    for ( int idx=0; idx<qlist.size(); idx++ )
	list.add( mQStringToConstChar(qlist[idx]) );
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
		if ( !isalnum(*fltptr) )
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

    deepErase( filenames_ );
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
	BufferString fname( externalfilenames_[idx] );
	FilePath fp( fname );
	if ( !fp.isAbsolute() )
	    fp = FilePath( dir, fname );
	fname = fp.fullPath();

	if ( !idx && externalfilenames_->size()>1 && mode_!=ExistingFiles )
	    mRetErrMsg( fname, "expected to be solitary" );

	if ( File::isDirectory(fname) )
	{
	    if ( mode_!=Directory && mode_!=DirectoryOnly )
		mRetErrMsg( fname, "specifies an existing directory" );
	    if ( !forread_ && !File::isWritable(fname) )
		mRetErrMsg( fname, "specifies a read-only directory" );
	}
	else 
	{
	    if ( mode_==Directory || mode_==DirectoryOnly )
		mRetErrMsg( fname, "specifies no existing directory" );

	    if ( !File::exists(fname) )
	    {
		if ( mode_ != AnyFile ) 
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
	replaceCharacter( bs->buf(), '/', '\\' );
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
	mRetMsg( externalfilenames_[wrongextidx],
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

    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( carrier )
#ifdef __lux32__
	carrier->endCmdRecEvent( (od_uint32) this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_uint64) this, refnr, msg );
#endif

}




