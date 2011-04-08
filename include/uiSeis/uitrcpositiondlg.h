#ifndef uitrcpositiondlg_h
#define uitrcpositiondlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
 RCS:           $Id: uitrcpositiondlg.h,v 1.3 2011-04-08 12:45:40 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "cubesampling.h"
#include "linekey.h"
#include "multiid.h"

class uiLabeledSpinBox;
class uiSpinBox;
class uiLabeledComboBox;

mClass uiTrcPositionDlg: public uiDialog
{                                                                               
public:                                                                         
				uiTrcPositionDlg(uiParent*,const CubeSampling&,
			      			 bool,const MultiID&);                   
    CubeSampling		getCubeSampling() const;
    LineKey			getLineKey() const;                        
    uiLabeledSpinBox*		trcnrfld_;                                 
    uiLabeledSpinBox*		inlfld_;                                  
    uiSpinBox*			crlfld_;    
    uiLabeledComboBox*		linesfld_;                                          MultiID			mid_;                                   

protected:                                                                      
    void			lineSel(CallBacker*);
    StepInterval<float>		zrg_;
};

#endif
