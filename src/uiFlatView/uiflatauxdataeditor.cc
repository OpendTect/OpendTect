/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.cc,v 1.5 2008-11-14 04:43:52 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiflatauxdataeditor.h"

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"

uiFlatViewAuxDataEditor::uiFlatViewAuxDataEditor( uiFlatViewer& vw )
    : FlatView::AuxDataEditor(vw,vw.rgbCanvas().getMouseEventHandler())
{
    vw.rgbCanvas().reDrawNeeded.notify(
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
}


void uiFlatViewAuxDataEditor::viewChangeCB( CallBacker* cb )
{
    if ( mousedown_ ) return;

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
