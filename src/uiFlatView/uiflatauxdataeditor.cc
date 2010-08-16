/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatauxdataeditor.cc,v 1.11 2010-08-16 14:45:23 cvsjaap Exp $";

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
    if ( uivw )
       setView( uivw->curView(), uivw->rgbCanvas().arrArea() );
}


void uiFlatViewAuxDataEditor::sizeChangeCB( CallBacker* )
{
    if ( mousedown_ ) return;

    mDynamicCastGet(uiFlatViewer*,uivw,&viewer_);
    if ( uivw ) mousearea_ = uivw->rgbCanvas().arrArea();
}
