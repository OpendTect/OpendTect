
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultsticksetflatvieweditor.cc,v 1.1 2010-02-12 08:42:13 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "emfault3dpainter.h"
#include "emfaultstickpainter.h"
#include "emmanager.h"
#include "flatauxdataeditor.h"

namespace EM
{

FaultStickSetFlatViewEditor::FaultStickSetFlatViewEditor(
				FlatView::AuxDataEditor* editor )
{
    cs_.setEmpty();
    fsspainter_ = new FaultStickPainter( editor->viewer() );
    f3dpainter_ = new Fault3DPainter( editor->viewer() );
}


void FaultStickSetFlatViewEditor::setCubeSampling( const CubeSampling& cs )
{ 
    cs_ = cs;
    fsspainter_->setCubeSampling( cs, true ); 
    f3dpainter_->setCubeSampling( cs, true );
}


void FaultStickSetFlatViewEditor::drawFault()
{
    for ( int idx=0; idx<EM::EMM().nrLoadedObjects(); idx++ )
    {
	//TODO better control
	fsspainter_->addFaultStickSet( EM::EMM().objectID(idx) );
	f3dpainter_->addFault3D( EM::EMM().objectID(idx) );
    }
}

} //namespace EM
