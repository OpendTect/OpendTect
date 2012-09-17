#ifndef visvw2dseismic_h
#define visvw2dseismic_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dseismic.h,v 1.1 2010/06/24 08:37:17 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"


mClass VW2DSeis : public Vw2DDataObject
{
public:
    			VW2DSeis();
			~VW2DSeis(){}

protected:

    void		triggerDeSel();

    Notifier<VW2DSeis>	deselted_;
};


#endif
