#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id: uihorinterpol.h,v 1.3 2009-05-16 03:45:32 cvskris Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "uidialog.h"

class uiGenInput;
namespace EM { class Horizon3D; }

class Array2DInterpol;
class uiArray2DInterpolSel;
class uiIOObjSel;


mClass uiHorizon3DInterpolDlg : public uiDialog
{
public:
    			uiHorizon3DInterpolDlg(uiParent*,EM::Horizon3D*);
			~uiHorizon3DInterpolDlg();

    const char*		helpID() const;

protected:
    void			saveChangeCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			expandArraysToSurvey();

    uiIOObjSel*			inputhorsel_;
    uiArray2DInterpolSel*	interpolsel_;

    uiGenInput*			geometrysel_;
    uiGenInput*			savefld_;
    uiIOObjSel*			outputfld_;

    EM::Horizon3D*		horizon_;
};


#endif
