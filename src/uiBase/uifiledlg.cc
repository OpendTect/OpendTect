/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.5 2001-08-31 15:21:27 arend Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "filegen.h"
#include <qfiledialog.h> 

uiFileDialog::uiFileDialog( uiParent* parnt, bool forread,
			    const char* fname, const char* filter,
			    const char* caption )
	: UserIDObject( "uiFileDialog" )
	, forread_( forread )
	, fname_( fname )
	, filter_( filter )
	, caption_( caption )
{
}


int uiFileDialog::go()
{
    QString filnm;

    if ( !File_exists(fname_) && !File_isDirectory(fname_) )
    {
	BufferString tmp( File_getPathOnly(fname_) ); 
	if ( !File_isDirectory( tmp ) )
	    fname_ = getenv( "HOME" );
    }

    if ( forread_ )
    {
	filnm =  QFileDialog::getOpenFileName( QString(fname_), 
			 QString(filter_), 0, name(), QString(caption_) );
    }
    else
	filnm =  QFileDialog::getSaveFileName( QString(fname_), 
			 QString(filter_), 0, name(), QString(caption_) );
    if ( filnm.isNull() ) return 0;

    fn = filnm;
    return 1;
}
