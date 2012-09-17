
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultsticksetflatvieweditor.cc,v 1.2 2010/03/16 07:15:14 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "flatauxdataeditor.h"

namespace EM
{

FaultStickSetFlatViewEditor::FaultStickSetFlatViewEditor(
				FlatView::AuxDataEditor* editor )
{
    cs_.setEmpty();
}


void FaultStickSetFlatViewEditor::setCubeSampling( const CubeSampling& cs )
{ cs_ = cs; }

} //namespace EM
