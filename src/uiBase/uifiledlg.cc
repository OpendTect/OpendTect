/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.21 2005-01-25 13:30:47 nanne Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "filegen.h"
#include "filepath.h"
#include "uiparentbody.h"


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
				const char* name=0, bool modal = FALSE )
				: QFileDialog( dirName, filter, parent, name,
					       modal )
				{}

			    dgbQFileDialog( QWidget* parent=0,
				const char* name=0, bool modal = FALSE )
				: QFileDialog( parent, name, modal ) {}


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
	: UserIDObject( "uiFileDialog" )
	, mode_( forread ? ExistingFile : AnyFile )
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
	, oktxt_( "Select" )
	, parnt_( parnt )
{
    if( !caption || !*caption )
	caption_ = forread ? "Open" : "Save As";
}

uiFileDialog::uiFileDialog( uiParent* parnt, Mode mode,
			    const char* fname, const char* filter,
			    const char* caption )
	: UserIDObject( "uiFileDialog" )
	, mode_( mode )
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
	, oktxt_( "Select" )
	, parnt_( parnt )
{}


int uiFileDialog::go()
{
    if ( !File_exists(fname_) && !File_isDirectory(fname_) )
    {
	if ( !File_isDirectory( FilePath(fname_).pathOnly() ) )
	    fname_ = GetPersonalDir();
    }

    QWidget* qp =0;
    if( parnt_ )
    { qp = parnt_->pbody() ? parnt_->pbody()->managewidg() : 0; }

    dgbQFileDialog* fd = new dgbQFileDialog( qp, name(), TRUE );

    fd->setMode( qmodeForUiMode(mode_) );
    fd->setFilters( QString(filter_) );
    fd->setCaption( QString(caption_) );
    fd->setDir( QString(fname_) );

    if ( oktxt_ != "" ) fd->okB->setText( (const char*)oktxt_ );
    if ( cnclxt_ != "") fd->cancelB->setText( (const char*)cnclxt_ );

    if ( fd->exec() != QDialog::Accepted )
	return 0;


    QStringList list = fd->selectedFiles();
    fn = list.size() ? list[0] : fd->selectedFile();
    selectedfilter = fd->selectedFilter();

#ifdef __win__
    replaceCharacter( fn.buf(), '/', '\\' );
#endif

    for ( int idx=0; idx<list.size(); idx++ )
    {
	BufferString* bs = new BufferString( list[idx] );
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

    string = qlist.join( (QString)filesep );
}


void uiFileDialog::string2List( const BufferString& string,
				BufferStringSet& list )
{
    QStringList qlist = QStringList::split( (QString)filesep, (QString)string );
    for ( int idx=0; idx<qlist.size(); idx++ )
	list += new BufferString( qlist[idx] );
}
