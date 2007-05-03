/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.35 2007-05-03 09:01:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "uiparentbody.h"

// Needed to change "Ok" and "Cancel" texts.
#define private public
#define protected public
#include <qfiledialog.h>
#undef private
#undef public
#include <qpushbutton.h>

const char* uiFileDialog::filesep = ";";

class ODFileDialog : public QFileDialog
{
public:
			    ODFileDialog( const QString& dirname,
				const QString& filter=QString::null,
				QWidget* parent=0,
				const char* caption=0, bool modal=false )
#ifndef USEQT3
				: QFileDialog(parent,caption,dirname,filter)
			    { setModal( modal ); }
#else
				: QFileDialog( dirname, filter, parent, caption,
				       	       modal )
			    {}
#endif

			    ODFileDialog( QWidget* parent=0,
					    const char* caption=0,
					    bool modal=false )
#ifndef USEQT3
				: QFileDialog( parent, caption )
			    { setModal( modal ); }
#else
				: QFileDialog( parent, caption, modal )
			    {}
#endif

};


QFileDialog::Mode qmodeForUiMode( uiFileDialog::Mode mode )
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


uiFileDialog::uiFileDialog( uiParent* parnt, bool forread,
			    const char* fname, const char* filter,
			    const char* caption )
	: mode_( forread ? ExistingFile : AnyFile )
        , forread_( forread )
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
	, oktxt_( "Select" )
	, parnt_( parnt )
    	, addallexts_( false )
{
    if ( !caption || !*caption )
	caption_ = forread ? "Open" : "Save As";
}


uiFileDialog::uiFileDialog( uiParent* parnt, Mode mode,
			    const char* fname, const char* filter,
			    const char* caption )
	: mode_( mode )
        , forread_(true)
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
	, oktxt_( "Select" )
	, parnt_( parnt )
    	, addallexts_( false )
{}

#ifdef USEQT3
# define mSelectFilter	setSelectedFilter
# define mSetDir	setDir
#else
# define mSelectFilter	selectFilter
# define mSetDir	setDirectory
#endif


int uiFileDialog::go()
{
    FilePath fp( fname_ );
    fname_ = fp.fullPath();
    BufferString dirname;
    if ( !File_isDirectory(fname_) )
    {
	if ( !File_isDirectory(fp.pathOnly()) )
	{
	    dirname = GetPersonalDir();
	    fname_ = "";
	}
	else if ( !File_exists(fname_) &&
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
	    flt += ".*";
#endif
	    flt += ")";
	}
    }

    ODFileDialog* fd = new ODFileDialog( QString(dirname), QString(flt),
					 qparent, "File dialog", true );
    fd->selectFile( QString(fname_) );
    fd->setAcceptMode( forread_ ? QFileDialog::AcceptOpen
	    			: QFileDialog::AcceptSave );
    fd->setMode( qmodeForUiMode(mode_) );
    fd->setCaption( QString(caption_) );
    if ( !currentdir_.isEmpty() )
	fd->mSetDir( QString(currentdir_.buf()) );
    if ( selectedfilter_.size() )
	fd->mSelectFilter( QString(selectedfilter_) );
    
#ifdef USEQT3
    if ( !oktxt_.isEmpty() ) fd->okB->setText( (const char*)oktxt_ );
    if ( !cnclxt_.isEmpty()) fd->cancelB->setText( (const char*)cnclxt_ );
#endif

#ifdef __win__
    fd->setViewMode( QFileDialog::Detail );
#endif

    if ( fd->exec() != QDialog::Accepted )
	return 0;

    QStringList list = fd->selectedFiles();
#ifndef USEQT3
    if (  list.size() )
	fn = list[0].toAscii().constData();
    else 
	fn = fd->selectedFile().toAscii().constData();

    selectedfilter_ = fd->selectedFilter().toAscii().constData();
#else
    fn = list.size() ? list[0] : fd->selectedFile();
    selectedfilter_ = fd->selectedFilter();
#endif
    
#ifdef __win__
    replaceCharacter( fn.buf(), '/', '\\' );
#endif

    for ( int idx=0; idx<list.size(); idx++ )
    {
#ifdef USEQT3
	BufferString* bs = new BufferString( list[idx] );
#else
	BufferString* bs = new BufferString( list[idx].toAscii().constData() );
#endif

#ifdef __win__
	replaceCharacter( bs->buf(), '/', '\\' );
#endif
	filenames += bs;
    }

    return 1;
}


void uiFileDialog::getFileNames( BufferStringSet& fnms ) const
{
    deepCopy( fnms, filenames );
}


void uiFileDialog::list2String( const BufferStringSet& list,
				BufferString& string )
{
    QStringList qlist;
    for ( int idx=0; idx<list.size(); idx++ )
	qlist.append( (QString)list[idx]->buf() );

#ifdef USEQT3
    string = qlist.join( (QString)filesep );
#else
    string = qlist.join( (QString)filesep ).toAscii().constData();
#endif
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
{
    QStringList qlist = QStringList::split( (QString)filesep, (QString)string );
    for ( int idx=0; idx<qlist.size(); idx++ )
#ifdef USEQT3
	list += new BufferString( qlist[idx] );
#else
	list += new BufferString( qlist[idx].toAscii().constData() );
#endif
}
