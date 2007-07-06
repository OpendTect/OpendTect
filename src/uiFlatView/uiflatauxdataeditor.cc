/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.cc,v 1.2 2007-07-06 16:51:26 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uiflatauxdataeditor.h"

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"

uiFlatViewAuxDataEditor::uiFlatViewAuxDataEditor( uiFlatViewer& vw )
    : FlatView::AuxDataEditor( vw,
	    		       vw.rgbCanvas().getMouseEventHandler() )
{
    vw.rgbCanvas().newFillNeeded.notify(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    vw.viewChanged.notify( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    vw.viewChanged.notify( mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );

    viewChangeCB( 0 );
    sizeChangeCB( 0 );
}


uiFlatViewAuxDataEditor::~uiFlatViewAuxDataEditor()
{
    mDynamicCastGet( uiFlatViewer&, uivw, viewer_ );
    uivw.viewChanged.remove( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    uivw.rgbCanvas().newFillNeeded.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
}


void uiFlatViewAuxDataEditor::viewChangeCB( CallBacker* cb )
{
    if ( mousedown_ ) return;
    
    mDynamicCastGet( uiFlatViewer&, uivw, viewer_ );
    curview_ = uivw.curView();
}


void uiFlatViewAuxDataEditor::sizeChangeCB( CallBacker* )
{
    if ( mousedown_ )
	return;

    mDynamicCastGet( uiFlatViewer&, uivw, viewer_ );
    mousearea_ = uivw.rgbCanvas().arrArea();
}
