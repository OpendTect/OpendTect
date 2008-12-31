#ifndef horizonmodifier_h
#define horizonmodifier_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonmodifier.h,v 1.4 2008-12-31 09:08:40 cvsranojay Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"

namespace EM { class Horizon; }
class HorSampling;

mClass HorizonModifier
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

    EM::Horizon*		tophor_;
    EM::Horizon*		bothor_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};


#endif
