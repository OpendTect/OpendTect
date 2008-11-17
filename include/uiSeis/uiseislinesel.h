
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.5 2008-11-17 13:04:40 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
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
						  CtxtIOObj* lsctio,
						  BoolTypeSet& lineselsum);
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
    TypeSet< StepInterval<int> >	linetrcflrgs_;
    BoolTypeSet&                 	linechksum_;

    void 				lineSetSel(CallBacker*);
    void 				trcRangeSel(CallBacker*);
    void				trc0Changed(CallBacker*);
    void				trc1Changed(CallBacker*);

    virtual bool        		acceptOK(CallBacker*);
};


class uiSelection2DParSel : public uiCompoundParSel
{
public:
    				uiSelection2DParSel(uiParent*);
				~uiSelection2DParSel();

    BufferString                getSummary() const;
    void                        doDlg(CallBacker*);
    IOObj*                      getIOObj();
    const BufferStringSet&      getSelLines() const     { return sellines_; }

protected:

    BufferStringSet             sellines_;
    CtxtIOObj*                  lsctio_;
    uiLineSel*                  linesel_;
    BoolTypeSet                 linechksum_;
};   

#endif
