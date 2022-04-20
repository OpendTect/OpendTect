#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
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
