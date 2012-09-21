#ifndef visvw2dseismic_h
#define visvw2dseismic_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "visvw2ddata.h"


mClass(uiViewer2D) VW2DSeis : public Vw2DDataObject
{
public:
    			VW2DSeis();
			~VW2DSeis(){}

protected:

    void		triggerDeSel();

    Notifier<VW2DSeis>	deselted_;
};


#endif

