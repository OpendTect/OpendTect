/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "flatauxdataeditor.h"

namespace EM
{

FaultStickSetFlatViewEditor::FaultStickSetFlatViewEditor(
					FlatView::AuxDataEditor* editor )
{
    tkzs_.setEmpty();
}


FaultStickSetFlatViewEditor::~FaultStickSetFlatViewEditor()
{}


void FaultStickSetFlatViewEditor::setTrcKeyZSampling(
					const TrcKeyZSampling& cs )
{
    tkzs_ = cs;
}

} // namespace EM
