#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

namespace View2D
{

mExpClass(uiViewer2D) Seismic : public DataObject
{
public:
			Seismic();
			~Seismic(){}

    NotifierAccess*	deSelection() override	{ return &deselected_; }

protected:

    void		triggerDeSel() override;

    Notifier<Seismic>	deselected_;
};

} // namespace View2D
