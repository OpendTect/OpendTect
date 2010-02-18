#ifndef uieditpdf_h
#define uieditpdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uieditpdf.h,v 1.1 2010-02-18 16:07:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class ProbDenFunc;
class uiGenInput;


/*! \brief Edit Probability Density Function */

mClass uiEditProbDenFunc : public uiDialog
{
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);
			~uiEditProbDenFunc();

protected:

    ProbDenFunc&		pdf_;
    const bool			editable_;

    ObjectSet<uiGenInput>	nmflds_;

    bool			acceptOK(CallBacker*);
};


#endif
