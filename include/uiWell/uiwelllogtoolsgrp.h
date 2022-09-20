#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"

// These classes are still under development. We don't guarantee binary
// compatibility.

class WellLogToolData;
class uiWellLogDisplay;


mExpClass(uiWell) uiWellLogToolWinGrp : public uiGroup
{ mODTextTranslationClass(uiWellLogToolWinGrp)
public:
				uiWellLogToolWinGrp(uiParent*,
					const ObjectSet<WellLogToolData>&);
    virtual			~uiWellLogToolWinGrp();

    virtual void		displayLogs() = 0;
    virtual void		displayMarkers(const BufferStringSet& mrknms) {}

protected:
    const ObjectSet<WellLogToolData>& logdatas_;
};


mExpClass(uiWell) uiODWellLogToolWinGrp : public uiWellLogToolWinGrp
{ mODTextTranslationClass(uiODWellLogToolWinGrp)
public:
			uiODWellLogToolWinGrp(uiParent*,
				const ObjectSet<WellLogToolData>&);
			~uiODWellLogToolWinGrp();

    void		displayLogs() override;

protected:
    ObjectSet<uiWellLogDisplay> logdisps_;
    Interval<float>		zdisplayrg_;

};
