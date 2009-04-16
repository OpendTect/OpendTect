#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id: uihorinterpol.h,v 1.1 2009-04-16 20:22:32 cvskris Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "uidialog.h"

class uiGenInput;
namespace EM { class Horizon3D; }

class Array2DInterpol;
class uiArray2DInterpolSel;


mClass uiHorizon3DInterpolDlg : public uiDialog
{
public:
    			uiHorizon3DInterpolDlg(uiParent*,EM::Horizon3D&);
			~uiHorizon3DInterpolDlg();

    const char*		helpID() const		{ return 0; }

protected:
    bool			acceptOK(CallBacker*);
    bool			expandArraysToSurvey();

    uiArray2DInterpolSel*	interpolsel_;

    uiGenInput*			geometrysel_;
    EM::Horizon3D&		horizon_;
};


#endif
