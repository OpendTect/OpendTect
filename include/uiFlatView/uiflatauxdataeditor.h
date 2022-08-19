#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
