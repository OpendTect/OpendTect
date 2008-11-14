
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.2 2008-11-14 11:28:39 cvsumesh Exp $
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

class BufferStringSet;
class CtxtIOObj;


class uiLineSel : public uiDialog
{
public:
    					uiLineSel(uiParent*,
						  BufferStringSet& selln,
						  CtxtIOObj* lsctio);
					~uiLineSel();
    BufferString			getSummary() const;

    const BufferStringSet&		getSelLines() const { return sellines_;}
    MultiID				getLineSetKey();
    IOObj*                      	getIOObj();

protected:

    int					nroflines_;				
    BufferStringSet& 			sellines_;				
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
