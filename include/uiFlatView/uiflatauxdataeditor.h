#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.h,v 1.3 2009-07-22 16:01:21 cvsbert Exp $
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
