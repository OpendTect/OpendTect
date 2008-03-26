#ifndef uistatsdisplaywin_h
#define uistatsdisplaywin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uistatsdisplaywin.h,v 1.1 2008-03-26 13:21:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class uiStatsDisplay;
namespace Stats { template <class T> class RunCalc; }

/*!\brief Stats display main window. See uistatsdisplay.h for details. */

class uiStatsDisplayWin : public uiMainWin
{
public:

    struct Setup
    {
				Setup( bool ismodal=false )
				    : modal_(ismodal)
				    , withplot_(true)
				    , withtext_(true)	{}

	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,withplot)
	mDefSetupMemb(bool,withtext)
    };

    				uiStatsDisplayWin(uiParent*,const Setup&);

    uiStatsDisplay&		statsDisplay()		{ return disp_; }
    void			setData(const Stats::RunCalc<float>&);
    void			setDataName(const char*);

protected:

    uiStatsDisplay&		disp_;

};


#endif
