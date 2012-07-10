/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatauxdataeditor.cc,v 1.16 2012-07-10 13:27:27 cvsbruno Exp $";

#include "uiflatauxdataeditor.h"

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"
#include "uigraphicsscene.h"

uiFlatViewAuxDataEditor::uiFlatViewAuxDataEditor( uiFlatViewer& vw )
    : FlatView::AuxDataEditor(vw,vw.rgbCanvas().scene().getMouseEventHandler())
    , uivwr_(vw)
{
    vw.rgbCanvas().reDrawNeeded.notify(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    vw.rgbCanvas().reSize.notify(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    vw.viewChanged.notify( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );

    viewChangeCB( 0 );
}


uiFlatViewAuxDataEditor::~uiFlatViewAuxDataEditor()
{
    uivwr_.viewChanged.remove( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    uivwr_.rgbCanvas().reDrawNeeded.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    uivwr_.rgbCanvas().reSize.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
}


void uiFlatViewAuxDataEditor::viewChangeCB( CallBacker* cb )
{
    setView( uivwr_.curView(), uivwr_.getViewRect() );
}


void uiFlatViewAuxDataEditor::sizeChangeCB( CallBacker* )
{
    if ( mousedown_ ) return;

    viewChangeCB( 0 );
}
