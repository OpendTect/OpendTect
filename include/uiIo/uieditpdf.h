#ifndef uieditpdf_h
#define uieditpdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uieditpdf.h,v 1.3 2010-02-27 10:44:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class ProbDenFunc;
class uiGenInput;
class uiTabStack;
class uiTable;


/*! \brief Edit Probability Density Function */

mClass uiEditProbDenFunc : public uiDialog
{
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);
			~uiEditProbDenFunc();

    bool		isChanged() const	{ return chgd_; }

protected:

    ProbDenFunc&		pdf_;
    const bool			editable_;
    bool			chgd_;

    uiTabStack*			tabstack_;
    ObjectSet<uiGenInput>	nmflds_;
    ObjectSet<uiTable>		tbls_;

    bool			acceptOK(CallBacker*);
};


#endif
