/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.cc,v 1.2 2001-05-16 14:57:20 arend Exp $
________________________________________________________________________

-*/

#include "uifiledlg.h"
#include "i_qobjwrap.h" 
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
    if ( forread_ )
	filnm =  QFileDialog::getOpenFileName( QString(fname_), 
			 QString(filter_), 0, name(), QString(caption_) );
    else
	filnm =  QFileDialog::getSaveFileName( QString(fname_), 
			 QString(filter_), 0, name(), QString(caption_) );
    if ( filnm.isNull() ) return 0;

    fn = filnm;
    return 1;
}
