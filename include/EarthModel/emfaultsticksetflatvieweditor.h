#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~FaultStickSetFlatViewEditor();

    virtual void	setTrcKeyZSampling(const TrcKeyZSampling&);
    virtual void	drawFault() =0;

protected:
			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*);

    TrcKeyZSampling	tkzs_;
};

} // namespace EM
