/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.7 2001-08-23 14:59:17 windev Exp $
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
{
    setWithSelect( true );
}


void uiFileInput::setFileName( const char* s )
{
    setText( s );
}


void uiFileInput::doSelect( CallBacker* )
{
    fname = text();
    uiFileDialog dlg( this, forread, fname, filter );
    if ( dlg.go() )
	setFileName( dlg.fileName() );
}


const char* uiFileInput::fileName()
{
    return text();
}
