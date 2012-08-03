#ifndef uiseedpropdlg_h
#define uiseedpropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseedpropdlg.h,v 1.4 2012-08-03 13:01:18 cvskris Exp $
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uimarkerstyledlg.h"
#include "emobject.h"

class uiColorInput;
class uiGenInput;
class uiSliderExtra;

mClass(uiVis) uiSeedPropDlg : public uiMarkerStyleDlg
{
public:
    			uiSeedPropDlg(uiParent*,EM::EMObject*);
			
protected:

    EM::EMObject*	emobject_;
    MarkerStyle3D	markerstyle_;

    void		doFinalise(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
    void		updateMarkerStyle();
};

#endif

