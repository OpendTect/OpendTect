
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.1 2008-11-14 06:43:15 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "multiid.h"
#include "sets.h"
#include "ranges.h"

class uiSeisSel;
class uiListBox;
class uiLabeledSpinBox;
class uiSpinBox;
class uiLabeledSpinBox;
class IOObj;

class CtxtIOObj;


class uiLineSel : public uiDialog
{
public:
    					uiLineSel(uiParent*);
					~uiLineSel();
    BufferString			getSummary() const;				
    const BufferStringSet&		getSelLines()	{ return sellines_; }
    MultiID				getLineSetKey(); 
    IOObj*                      	getIOObj();   

protected:

    int					nroflines_;				
    BufferStringSet 			sellines_;				
    uiSeisSel*  			linesetfld_;
    uiListBox*  			lnmsfld_;
    uiLabeledSpinBox*          	        lsb_;
    uiSpinBox*                          trc0fld_;
    uiSpinBox*                  	trc1fld_;
    CtxtIOObj*				lsctio_;
    TypeSet< StepInterval<int> > 	linetrcrgs_;

    void 			lineSetSel(CallBacker*);
    void 			trcRangeSel(CallBacker*);

    virtual bool        	acceptOK(CallBacker*);
};


#endif
