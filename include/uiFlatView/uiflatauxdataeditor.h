#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditor.h,v 1.4 2011/03/24 04:40:22 cvsranojay Exp $
________________________________________________________________________

-*/

#include "flatauxdataeditor.h"

class uiFlatViewer;

mClass uiFlatViewAuxDataEditor : public FlatView::AuxDataEditor
{
public:
		uiFlatViewAuxDataEditor(uiFlatViewer&);
		~uiFlatViewAuxDataEditor();

    uiFlatViewer&	getFlatViewer() const { return uivwr_; }
protected:
    void	viewChangeCB(CallBacker*);
    void	sizeChangeCB(CallBacker*);

    uiFlatViewer& uivwr_;
};

#endif
