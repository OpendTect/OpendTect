#ifndef uidpsrefineseldlg_h
#define uidpsrefineseldlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2011
 RCS:           $Id: uidpsrefineseldlg.h,v 1.1 2011/07/05 09:24:05 cvssatyaki Exp $: 
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class uiDataPointSetCrossPlotter;

class uiGenInput;
class uiPushButton;
class uiTable;

class MathExpression;

mClass uiDPSRefineSelDlg : public uiDialog
{
public:

				uiDPSRefineSelDlg(uiDataPointSetCrossPlotter&);
    MathExpression*		mathObject()		{ return mathobj_; }

protected:

    int				cColIds(int dcolid);
    void			updateDisplay();
    void			setPlotter();
    
    void			checkMathExpr(CallBacker*);
    void			parsePush(CallBacker*);
    bool			acceptOK(CallBacker*);
    
    uiDataPointSetCrossPlotter&	plotter_;
    BufferString		mathexprstring_;
    BufferStringSet		colnms_;
    MathExpression*		mathobj_;
    TypeSet<int>		dcolids_;
    
    uiGenInput*			inpfld_;
    uiPushButton*		setbut_;
    uiTable*			vartable_;
};

#endif
