/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.cc,v 1.1 2007-04-04 18:19:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatauxdataeditor.h"

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"

uiFlatViewAuxDataEditor::uiFlatViewAuxDataEditor( uiFlatViewer& viewer )
    : FlatView::AuxDataEditor( viewer,
	    		       viewer.rgbCanvas().getMouseEventHandler() )
{
    viewer.rgbCanvas().newFillNeeded.notify(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
    viewer.viewChanged.notify( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    viewer.viewChanged.notify( mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );

    viewChangeCB( 0 );
    sizeChangeCB( 0 );
}


uiFlatViewAuxDataEditor::~uiFlatViewAuxDataEditor()
{
    mDynamicCastGet( uiFlatViewer&, viewer, viewer_ );
    viewer.viewChanged.remove( mCB(this,uiFlatViewAuxDataEditor,viewChangeCB) );
    viewer.rgbCanvas().newFillNeeded.remove(
	    mCB(this,uiFlatViewAuxDataEditor,sizeChangeCB) );
}


void uiFlatViewAuxDataEditor::viewChangeCB( CallBacker* cb )
{
    if ( mousedown_ ) return;
    
    mDynamicCastGet( uiFlatViewer&, viewer, viewer_ );
    curview_ = viewer.curView();
}


void uiFlatViewAuxDataEditor::sizeChangeCB( CallBacker* )
{
    if ( mousedown_ )
	return;

    mDynamicCastGet( uiFlatViewer&, viewer, viewer_ );
    mousearea_ = viewer.rgbCanvas().arrArea();
}
