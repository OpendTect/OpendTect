/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.4 2001-05-02 07:39:01 bert Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include <datainpspec.h>


uiFileInput::uiFileInput( uiObject* p, const char* txt, const char* fnm,
			  bool fr, const char* filt )
	: uiGenInput( p, txt, FileNameInpSpec(fnm) )
	, forread(fr)
	, fname( fnm )
	, filter(filt)
{
    setWithSelect( true );
}


void uiFileInput::doSelect( CallBacker* )
{
    fname = text();
    uiFileDialog dlg( this, forread, fname, filter );
    if ( dlg.go() )
	setText( dlg.fileName() );
}


const char* uiFileInput::fileName()
{
    return text();
}
