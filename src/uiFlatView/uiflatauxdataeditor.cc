/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatauxdataeditor.cc,v 1.8 2009-06-23 06:24:07 cvssatyaki Exp $";

#include "uiflatauxdataeditor.h"

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"
#include "uigraphicsscene.h"

uiFlatViewAuxDataEditor::uiFlatViewAuxDataEditor( uiFlatViewer& vw )
    : FlatView::AuxDataEditor(vw,vw.rgbCanvas().scene().getMouseEventHandler())
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
    mDynamicCastGet(uiFlatViewer*,uivw,&viewer_);
    uivw->viewChanged.remove( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    uivw->rgbCanvas().reDrawNeeded.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    uivw->rgbCanvas().reSize.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
}


void uiFlatViewAuxDataEditor::viewChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewer*,uivw,&viewer_);
    if ( uivw ) curview_ = uivw->curView();

    sizeChangeCB( cb );
}


void uiFlatViewAuxDataEditor::sizeChangeCB( CallBacker* )
{
    if ( mousedown_ ) return;

    mDynamicCastGet(uiFlatViewer*,uivw,&viewer_);
    if ( uivw ) mousearea_ = uivw->rgbCanvas().arrArea();
}
