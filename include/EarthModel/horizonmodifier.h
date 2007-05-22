#ifndef horizonmodifier_h
#define horizonmodifier_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonmodifier.h,v 1.2 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"

namespace EM { class Horizon3D; }
class HorSampling;

class HorizonModifier
{
public:

				HorizonModifier();
				~HorizonModifier();

    enum ModifyMode		{ Shift, Remove };

    bool			setHorizons(const MultiID&,const MultiID&);
    void			setStaticHorizon(bool tophor);
    void			setMode(ModifyMode);

    void			doWork();

protected:

    void			getHorSampling(HorSampling&);
    void			shiftNode(const EM::SubID&);
    void			removeNode(const EM::SubID&);

    EM::Horizon3D*		tophor_;
    EM::Horizon3D*		bothor_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};


#endif
