#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.h,v 1.1 2007-04-04 18:19:49 cvskris Exp $
________________________________________________________________________

-*/

#include "flatauxdataeditor.h"

class uiFlatViewer;

class uiFlatViewAuxDataEditor : public FlatView::AuxDataEditor
{
public:
		uiFlatViewAuxDataEditor(uiFlatViewer&);
		~uiFlatViewAuxDataEditor();

protected:
    void	viewChangeCB(CallBacker*);
    void	sizeChangeCB(CallBacker*);
};

#endif
