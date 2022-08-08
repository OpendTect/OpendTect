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


mExpClass(uiViewer2D) VW2DSeis : public Vw2DDataObject
{
public:
			VW2DSeis();
			~VW2DSeis(){}

    NotifierAccess*	deSelection() override	{ return &deselted_; }

protected:

    void		triggerDeSel() override;

    Notifier<VW2DSeis>	deselted_;
};


