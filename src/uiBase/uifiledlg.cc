/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.8 2002-03-18 08:17:39 nanne Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "filegen.h"
#include <qfiledialog.h> 


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


    QFileDialog* fd = new QFileDialog( 0, name(), TRUE );

    fd->setMode( qmodeForUiMode(mode_) );// QFileDialog::ExistingFile );
    fd->setFilter( QString(filter_) );
    fd->setCaption( QString(caption_) );
    fd->setDir( QString(fname_) );


    if ( fd->exec() == QDialog::Accepted )
        filnm = fd->selectedFile();


    if ( filnm.isNull() ) return 0;

    fn = filnm;
    return 1;
}
