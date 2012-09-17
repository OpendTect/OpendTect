#ifndef emfaultsticksetflatvieweditor_h
#define emfaultsticksetflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultsticksetflatvieweditor.h,v 1.3 2010/08/04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"

class CubeSampling;

namespace FlatView { class AuxDataEditor; }

namespace EM
{

mClass FaultStickSetFlatViewEditor : public CallBacker
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*);
			~FaultStickSetFlatViewEditor() {}

    virtual void	setCubeSampling(const CubeSampling&);
    virtual void	drawFault() =0;    

protected:
    CubeSampling	cs_;			
};
} //namespace EM


#endif
