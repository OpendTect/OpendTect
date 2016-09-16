#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "flatauxdataeditor.h"

class uiFlatViewer;

/*!
\brief Flatview auxiliary data editor.
*/

mExpClass(uiFlatView) uiFlatViewAuxDataEditor : public FlatView::AuxDataEditor
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
