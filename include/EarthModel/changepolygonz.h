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

class TaskRunner;
namespace Pick { class Set; }
namespace EM {
    class		Object;
    class		ObjectManager;
    class		SurfaceIODataSelection;
}

mExpClass(EarthModel) ChangePolygonZ
{ mODTextTranslationClass(ChangePolygonZ);
public:
			ChangePolygonZ(Pick::Set&,const DBKey horid);
			ChangePolygonZ(Pick::Set&,double zval);
			~ChangePolygonZ();

    bool		doShift(const TaskRunnerProvider&);
    bool		shiftPickToHorizon(const TaskRunnerProvider&);
    bool		shiftPickToConstant();

protected:
    Pick::Set&		ps_;
    DBKey		horid_;
    void		changeZval();
    double		constzval_;
};
