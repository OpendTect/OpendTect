
#ifndef emfaultstickseteditor_h
#define emfaultstickseteditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultsticksetflatvieweditor.h,v 1.2 2010-03-16 07:15:08 cvsumesh Exp $
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
