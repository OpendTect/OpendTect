#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
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
