#ifndef uistatsdisplaywin_h
#define uistatsdisplaywin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id$
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

mClass(uiTools) uiStatsDisplayWin : public uiMainWin
{
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
};


#endif

