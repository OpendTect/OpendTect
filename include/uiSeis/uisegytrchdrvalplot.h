#ifndef uisegytrchdrvalplot_h
#define uisegytrchdrvalplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
 RCS:           $Id: uisegytrchdrvalplot.h,v 1.1 2011-03-10 12:35:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiLabel;
class uiFunctionDisplay;
namespace SEGY { class HdrEntry; }


mClass uiSEGYTrcHdrValPlot : public uiGroup
{
public:

    			uiSEGYTrcHdrValPlot(uiParent*,bool singlehdr=true);
    virtual		~uiSEGYTrcHdrValPlot();

    void		setData(const SEGY::HdrEntry&,
	    			const float*,int sz,bool first=true);

protected:

    bool				issingle_;

    uiLabel*				tlbl1_;
    uiLabel*				tlbl2_;
    uiLabel*				slbl1_;
    uiLabel*				slbl2_;
    uiFunctionDisplay*			disp_;

};


#endif
