/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.12 2003-01-20 14:31:34 arend Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include <datainpspec.h>


uiFileInput::uiFileInput( uiParent* p, const char* txt, const char* fnm,
			  bool fr, const char* filt )
    : uiGenInput( p, txt, FileNameInpSpec(fnm) )
    , forread(fr)
    , fname( fnm )
    , filter(filt)
    , newfltr(false)
    , selmodset(false)
    , selmode(uiFileDialog::AnyFile)
{
    setWithSelect( true );
}


void uiFileInput::setFileName( const char* s )
{
    setText( s );
    uiLineEdit* le = dynamic_cast<uiLineEdit*>(element());
    if( le ) le->end();
}


void uiFileInput::doSelect( CallBacker* )
{
    fname = text();
    if ( fname == "" )	fname = defseldir;
    if ( newfltr )	filter = selfltr;

    uiFileDialog dlg( this, forread, fname, filter );

    if ( selmodset )	dlg.setMode( selmode );
    if ( dlg.go() )
    {
	BufferString oldfname( fname );
	BufferString newfname( dlg.fileName() );
	setFileName( newfname );
	if ( newfname != oldfname )
	    valuechanged.trigger( *this );
    }
}


const char* uiFileInput::fileName()
{
    return text();
}
