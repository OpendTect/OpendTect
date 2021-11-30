#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Wayne Mogg
Date:	       November 2021
________________________________________________________________________

-*/


#include "uiwellmod.h"
#include "uigroup.h"
#include "uiwelllogtools.h"


mExpClass(uiWell) uiWellLogToolWinGrp : public uiGroup
{ mODTextTranslationClass(uiWellLogToolWinGrp)
public:
				uiWellLogToolWinGrp(uiParent*,
				const ObjectSet<uiWellLogToolWin::LogData>&);
    virtual			~uiWellLogToolWinGrp();

    virtual void		displayLogs() = 0;

protected:
    const ObjectSet<uiWellLogToolWin::LogData>& logdatas_;
};


mExpClass(uiWell) uiODWellLogToolWinGrp : public uiWellLogToolWinGrp
{ mODTextTranslationClass(uiODWellLogToolWinGrp)
public:
			uiODWellLogToolWinGrp(uiParent*,
				const ObjectSet<uiWellLogToolWin::LogData>&);
			~uiODWellLogToolWinGrp();

    void		displayLogs() override;

protected:
    ObjectSet<uiWellLogDisplay> logdisps_;
    Interval<float>		zdisplayrg_;

};
