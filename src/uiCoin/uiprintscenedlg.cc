/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.1 2002-10-16 07:34:21 kristofer Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "ptrman.h"

#include "Inventor/SoOffscreenRenderer.h"

uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p, SoNode* scene_ )
	: uiDialog(p, uiDialog::Setup("Print Scene",
		    		      "Enter filename and fileformat"))
	, scene(scene_)
{
    filetypesfld = new uiListBox(this, "Filetypes" );
    PtrMan<SoOffscreenRenderer> r =
	new SoOffscreenRenderer(*(new SbViewportRegion));
    int num = r->getNumWriteFiletypes();

    for ( int idx=0; idx<num; idx++ )
    {
	SbList<SbName> extlist;
	SbString fullname, description;
	r->getWriteFiletypeInfo(idx, extlist, fullname, description);
	filetypesfld->addItem( (const char*) fullname.getString() );
    }

    fileinputfld = new uiFileInput( this, "Select filename", 0, false,0 );
    fileinputfld->attach( alignedBelow, filetypesfld );
    finaliseStart.notify( mCB(this,uiPrintSceneDlg,doFinalise) );
}


void uiPrintSceneDlg::doFinalise( CallBacker* )
{ } 

bool uiPrintSceneDlg::acceptOK( CallBacker* )
{
    SbViewportRegion viewport;
    viewport.setWindowSize( SbVec2s( 3000, 4000 ));

    PtrMan<SoOffscreenRenderer> r = new SoOffscreenRenderer(viewport);

    const char* filename = fileinputfld->fileName();
    if ( !filename || !filename[0] )
    { return false; }

    int filetypenr = filetypesfld->currentItem();

    SbList<SbName> extlist;
    SbString fullname, description;
    r->getWriteFiletypeInfo(filetypenr, extlist, fullname, description);

    if ( !r->render( scene ) ) return false;
    return r->writeToFile( filename, extlist[0].getString() );
}
