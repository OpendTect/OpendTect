#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uimainwin.h"
#include "uistatsdisplay.h"
class BufferStringSet;
class uiComboBox;
class uiStatsDisplay;
namespace Stats { template <class T> class ParallelCalc; }

/*!\brief Stats display main window. See uistatsdisplay.h for details. */

mExpClass(uiTools) uiStatsDisplayWin : public uiMainWin
{ mODTextTranslationClass(uiStatsDisplayWin)
public:
    				uiStatsDisplayWin(uiParent*,
					const uiStatsDisplay::Setup&,int nr=1,
					bool ismodal=true);

    uiStatsDisplay*		statsDisplay(int nr=0)	{ return disps_[nr]; }
    void                        setData(const float* medarr,int medsz,int nr=0);
    void			addDataNames(const BufferStringSet&);
    void			setDataName(const char*,int nr=0);
    void			setMarkValue(float,bool forx,int nr=0);
    void			showStat(int);

protected:

    ObjectSet<uiStatsDisplay>	disps_;
    uiComboBox*			statnmcb_;

    void			dataChanged(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    int				currentdispidx_;
};
