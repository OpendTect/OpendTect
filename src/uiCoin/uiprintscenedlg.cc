/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.2 2002-10-17 06:00:23 kristofer Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"
#include "uifileinput.h"
#include "uicombobox.h"
#include "ptrman.h"

#include "Inventor/SoOffscreenRenderer.h"

uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p, SoNode* scene_ )
	: uiDialog(p, uiDialog::Setup("Print Scene",
		    		      "Enter filename and fileformat"))
	, scene(scene_)
{
    IntInpSpec horwidthinpspec( 800  );
    IntInpSpec vertwidthinpspec( 600 );
    IntInpSpec resinpspec( 72 );
    horwidthfld = new uiGenInput( this, "Size", horwidthinpspec );

    vertwidthfld = new uiGenInput( this, "x", vertwidthinpspec );
    vertwidthfld->attach( rightOf, horwidthfld );

    widthunitfld = new uiComboBox( this, "Unit" );
    widthunitfld->addItem("pixels");
    widthunitfld->addItem("cm");
    widthunitfld->addItem("inches");
    widthunitfld->attach( rightOf, vertwidthfld );

    resolutionfld = new uiGenInput( this, "Resolution (dpi)", resinpspec );
    resolutionfld->attach( alignedBelow, horwidthfld );

    filetypesfld = new uiComboBox(this, "Filetypes" );
    filetypesfld->attach( alignedBelow, resolutionfld );

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
    double horwidth = horwidthfld->getValue();
    double vertwidth = vertwidthfld->getValue();
    int widthunit = widthunitfld->currentItem();

    double resolution = resolutionfld->getValue();

    if ( widthunit==1 )
    {
	horwidth *= resolution/2.54;
	vertwidth *= resolution/2.54;
    }
    else if ( widthunit==2 )
    {
	horwidth *= resolution;
	vertwidth *= resolution;
    }
    
    SbViewportRegion viewport;
    SbVec2s maxres = SoOffscreenRenderer::getMaximumResolution();
    if ( horwidth>maxres[0] ) horwidth = maxres[0];
    if ( vertwidth>maxres[1] ) vertwidth = maxres[1];

    viewport.setWindowSize( SbVec2s( mNINT(horwidth), mNINT(vertwidth) ));
    viewport.setPixelsPerInch( resolution );

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
