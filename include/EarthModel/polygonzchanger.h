#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
________________________________________________________________________

-*/
#include "earthmodelmod.h"
#include "dbkey.h"

class TaskRunnerProvider;
namespace Pick { class Set; }
namespace EM {
  mExpClass(EarthModel) PolygonZChanger
{ mODTextTranslationClass(PolygonZChanger);
public:
			PolygonZChanger(Pick::Set&,const DBKey horid);
			PolygonZChanger(Pick::Set&,float zval);
			~PolygonZChanger();

    uiRetVal		doWork(const TaskRunnerProvider&) const;

protected:
    Pick::Set&		ps_;
    DBKey		horid_;
    void		changeZval();
    float		zval_;
};
}
