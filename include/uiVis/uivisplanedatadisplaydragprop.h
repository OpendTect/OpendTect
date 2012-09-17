#ifndef uivisplanedatadisplaydragprop_h
#define uivisplanedatadisplaydragprop_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: uivisplanedatadisplaydragprop.h,v 1.3 2009/07/22 16:01:24 cvsbert Exp $
________________________________________________________________________

*/

#include "uidlggroup.h"

namespace visSurvey { class PlaneDataDisplay; }

class uiButtonStateEdit;

mClass uiVisPlaneDataDisplayDragProp : public uiDlgGroup
{
public:
    			uiVisPlaneDataDisplayDragProp(uiParent*,
			    visSurvey::PlaneDataDisplay* );
    bool		acceptOK();
    bool		revertChanges();

protected:
    visSurvey::PlaneDataDisplay*	pdd_;
    uiButtonStateEdit*			scrollstate_;
    uiButtonStateEdit*			panstate_;

    const int				initialscrollstate_;
    const int				initialpanstate_;
};

#endif
