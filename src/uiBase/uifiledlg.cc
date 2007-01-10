/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.29 2007-01-10 15:58:54 cvsnanne Exp $
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

class dgbQFileDialog : public QFileDialog
{
public:
			    dgbQFileDialog( const QString& dirName,
				const QString& filter=QString::null,
				QWidget* parent=0,
				const char* name=0, bool modal=false )
#ifdef USEQT4
				: QFileDialog( parent, name, dirName, filter )
			    { setModal( modal ); }
#else
				: QFileDialog( dirName, filter, parent, name,
				       	       modal )
			    {}
#endif

			    dgbQFileDialog( QWidget* parent=0,
					    const char* name=0,
					    bool modal=false )
#ifdef USEQT4
				: QFileDialog( parent, name )
			    { setModal( modal ); }
#else
				: QFileDialog( parent, name, modal )
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
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
	, oktxt_( "Select" )
	, parnt_( parnt )
    	, addallexts_( false )
{}

#ifdef USEQT4
# define mSetFilter	setFilter
# define mSelectFilter	selectFilter
#else
# define mSetFilter	setFilters
# define mSelectFilter	setSelectedFilter
#endif


int uiFileDialog::go()
{
    if ( !File_exists(fname_) && !File_isDirectory(fname_) )
    {
	if ( !File_isDirectory( FilePath(fname_).pathOnly() ) )
	    fname_ = GetPersonalDir();
    }

    QWidget* qp =0;

    if ( parnt_ )
	{ qp = parnt_->pbody() ? parnt_->pbody()->managewidg() : 0; }

    dgbQFileDialog* fd = new dgbQFileDialog( qp, "File dialog", TRUE );

    fd->setMode( qmodeForUiMode(mode_) );
    if ( filter_.size() )
    {
	BufferString flt( filter_ );
	if ( addallexts_ )
	{
	    flt += ";;All files (*";
#ifdef __win__
	    flt += ".*";
#endif
	    flt += ")";
	}
	fd->mSetFilter( QString(flt) );
    }
    fd->setCaption( QString(caption_) );
    fd->setDir( QString(fname_) );
    if ( selectedfilter_.size() )
	fd->mSelectFilter( QString(selectedfilter_) );
    
#ifndef USEQT4
    if ( !oktxt_.isEmpty() ) fd->okB->setText( (const char*)oktxt_ );
    if ( !cnclxt_.isEmpty()) fd->cancelB->setText( (const char*)cnclxt_ );
#endif


    if ( fd->exec() != QDialog::Accepted )
	return 0;


    QStringList list = fd->selectedFiles();
#ifdef USEQT4
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
#ifdef USEQT4
	BufferString* bs = new BufferString( list[idx].toAscii().constData() );
#else
	BufferString* bs = new BufferString( list[idx] );
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

#ifdef USEQT4
    string = qlist.join( (QString)filesep ).toAscii().constData();
#else
    string = qlist.join( (QString)filesep );
#endif
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
{
    QStringList qlist = QStringList::split( (QString)filesep, (QString)string );
    for ( int idx=0; idx<qlist.size(); idx++ )
#ifdef USEQT4
	list += new BufferString( qlist[idx].toAscii().constData() );
#else
	list += new BufferString( qlist[idx] );
#endif
}
