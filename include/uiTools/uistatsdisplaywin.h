#ifndef uistatsdisplaywin_h
#define uistatsdisplaywin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uistatsdisplaywin.h,v 1.3 2008-04-02 10:57:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uistatsdisplay.h"
class uiStatsDisplay;
namespace Stats { template <class T> class RunCalc; }

/*!\brief Stats display main window. See uistatsdisplay.h for details. */

class uiStatsDisplayWin : public uiMainWin
{
public:

    				uiStatsDisplayWin(uiParent*,
						  const uiStatsDisplay::Setup&,
						  bool ismodal=false);

    uiStatsDisplay&		statsDisplay()		{ return disp_; }
    void			setData(const Stats::RunCalc<float>&);
    void			setDataName(const char*);
    void			setMarkValue(float,bool forx);

protected:

    uiStatsDisplay&		disp_;

};


#endif
