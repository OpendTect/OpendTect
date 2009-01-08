#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.h,v 1.2 2009-01-08 07:14:05 cvsranojay Exp $
________________________________________________________________________

-*/

#include "flatauxdataeditor.h"

class uiFlatViewer;

mClass uiFlatViewAuxDataEditor : public FlatView::AuxDataEditor
{
public:
		uiFlatViewAuxDataEditor(uiFlatViewer&);
		~uiFlatViewAuxDataEditor();

protected:
    void	viewChangeCB(CallBacker*);
    void	sizeChangeCB(CallBacker*);
};

#endif
