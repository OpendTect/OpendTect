/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.2 2001-01-24 12:58:58 arend Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigenselect.h"


uiFileInput::uiFileInput( uiObject* p, const char* txt, const char* fnm,
			  bool fr, bool withclear, const char* filt )
	: uiGenSelect( p, txt, fnm, 0, withclear )
	, forread(fr)
	, fname( fnm )
	, filter(filt)
{}


void uiFileInput::doSelect_( CallBacker* )
{
    fname = inpflds->text();
    uiFileDialog dlg( this, forread, fname, filter );
    if ( dlg.go() )
	inpflds->setText( dlg.fileName() );
}


const char* uiFileInput::fileName()
{
    return inpflds->text();
}
