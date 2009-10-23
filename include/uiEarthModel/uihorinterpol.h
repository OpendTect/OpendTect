#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id: uihorinterpol.h,v 1.5 2009-10-23 21:24:21 cvsyuancheng Exp $
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
    bool		displayNewHorizon() const;
    EM::Horizon3D* 	getNewHorizon() const	{ return newhorizon_; }

protected:
    void			saveChangeCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			expandArraysToSurvey();
    bool			createNewHorizon();

    uiIOObjSel*			inputhorsel_;
    uiArray2DInterpolSel*	interpolsel_;

    uiGenInput*			geometrysel_;
    uiGenInput*			savefld_;
    uiGenInput*			addnewfld_;
    uiIOObjSel*			outputfld_;

    EM::Horizon3D*		horizon_;
    EM::Horizon3D*		newhorizon_;
};


#endif
