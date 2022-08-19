#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "multiid.h"
#include "picksetmgr.h"
#include "task.h"

namespace Pick { class Set; }
namespace EM
{
mExpClass(EarthModel) PolygonZChanger
{ mODTextTranslationClass(PolygonZChanger)
public:
			PolygonZChanger(Pick::Set&,const MultiID& horid);
			PolygonZChanger(Pick::Set&,float zval);
			~PolygonZChanger();

    uiRetVal		doWork(TaskRunner&);

protected:
    void		changeZval();
    void		reportChange(Pick::SetMgr::ChangeData::Ev ev,int idy);

    RefMan<Pick::Set>	ps_;
    MultiID		horid_;
    float		zval_;

private:
    enum EventType	{ Remove, Move };
};

} // namespace EM
