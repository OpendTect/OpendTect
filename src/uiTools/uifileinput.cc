/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uifileinput.cc,v 1.1 2000-11-27 10:20:46 bert Exp $
________________________________________________________________________

-*/

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uilineedit.h"


uiFileInput::uiFileInput( uiObject* p, const char* txt, const char* fnm,
			  bool fr, bool withclear, const char* filt )
	: uiGroup( p, txt, 0 )
	, forread(fr)
	, fname( fnm )
	, filter(filt)
{
    inpfld = new uiLineEdit( this, fnm?fnm:"" );
    new uiLabel( this, txt, inpfld );
    uiPushButton* selbut = new uiPushButton( this, "Select ..." );
    selbut->notify( mCB(this,uiFileInput,doSelect) );
    selbut->attach( rightOf, inpfld );
    if ( withclear )
    {
	uiPushButton* clrbut = new uiPushButton( this, "Clear ..." );
	clrbut->attach( rightOf, selbut );
	clrbut->notify( mCB(this,uiFileInput,doClear) );
    }

    setHAlignObj( inpfld );
}


void uiFileInput::doSelect( CallBacker* )
{
    fname = inpfld->text();
    uiFileDialog dlg( this, forread, fname, filter );
    if ( dlg.go() )
	inpfld->setText( dlg.fileName() );
}


void uiFileInput::doClear( CallBacker* )
{
    inpfld->setText( "" );
}


const char* uiFileInput::fileName()
{
    return inpfld->text();
}
