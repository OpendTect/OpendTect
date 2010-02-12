
#ifndef emfaultstickseteditor_h
#define emfaultstickseteditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultsticksetflatvieweditor.h,v 1.1 2010-02-12 08:41:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"

class CubeSampling;

namespace FlatView { class AuxDataEditor; }

namespace EM
{

class FaultStickPainter;
class Fault3DPainter;

mClass FaultStickSetFlatViewEditor : public CallBacker
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*);
			~FaultStickSetFlatViewEditor() {}

    void		setCubeSampling(const CubeSampling&);
    void		drawFault();    

protected:

    FaultStickPainter*	fsspainter_;
    Fault3DPainter*	f3dpainter_;
    CubeSampling	cs_;
				
};
} //namespace EM


#endif
