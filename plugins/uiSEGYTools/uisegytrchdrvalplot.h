#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uigroup.h"
class uiLabel;
class uiFunctionDisplay;
namespace SEGY { class HdrEntry; }


mExpClass(uiSEGYTools) uiSEGYTrcHdrValPlot : public uiGroup
{ mODTextTranslationClass(uiSEGYTrcHdrValPlot)
public:

			uiSEGYTrcHdrValPlot(uiParent*,bool singlehdr=true,
					    int firsttrcnr=1);
    virtual		~uiSEGYTrcHdrValPlot();

    void		setData(const SEGY::HdrEntry&,
				const float*,int sz,bool first=true);

protected:

    bool		issingle_;
    int			trcnr0_;
    TypeSet<float>	xvals_;
    TypeSet<float>	yvals_;

    uiLabel*		tlbl1_;
    uiLabel*		tlbl2_;
    uiLabel*		slbl1_;
    uiLabel*		slbl2_;
    uiFunctionDisplay*	disp_;

    void		getBendPoints(const float*,int);

};
