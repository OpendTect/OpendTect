
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
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


void FaultStickSetFlatViewEditor::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{ tkzs_ = cs; }

} //namespace EM
