/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.11 2002-10-08 08:36:29 bert Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "filegen.h"


#define private public
#define protected public
#include <qfiledialog.h> 
#undef private
#undef public
#include <qpushbutton.h>

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
{}


int uiFileDialog::go()
{
    QString filnm;

    if ( !File_exists(fname_) && !File_isDirectory(fname_) )
    {
	BufferString tmp( File_getPathOnly(fname_) ); 
	if ( !File_isDirectory( tmp ) )
	    fname_ = GetHomeDir();
    }


    dgbQFileDialog* fd = new dgbQFileDialog( 0, name(), TRUE );

    fd->setMode( qmodeForUiMode(mode_) );
    fd->setFilters( QString(filter_) );
    fd->setCaption( QString(caption_) );
    fd->setDir( QString(fname_) );

    if ( oktxt_ != "" ) fd->okB->setText( (const char*)oktxt_ );
    if ( cnclxt_ != "") fd->cancelB->setText( (const char*)cnclxt_ );

    if ( fd->exec() == QDialog::Accepted )
        filnm = fd->selectedFile();


    if ( filnm.isNull() ) return 0;

    fn = filnm;
    return 1;
}
