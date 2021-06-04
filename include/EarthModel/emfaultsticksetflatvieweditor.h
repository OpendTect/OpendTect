#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "callback.h"
#include "trckeyzsampling.h"

class TrcKeyZSampling;

namespace FlatView { class AuxDataEditor; }

namespace EM
{

/*!
\brief %Fault stick set flat view editor.
*/

mExpClass(EarthModel) FaultStickSetFlatViewEditor : public CallBacker
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*);
			~FaultStickSetFlatViewEditor() {}

    virtual void	setTrcKeyZSampling(const TrcKeyZSampling&);
    virtual void	drawFault() =0;

protected:
    TrcKeyZSampling	tkzs_;
};

} //namespace EM

