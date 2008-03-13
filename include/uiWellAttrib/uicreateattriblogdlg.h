#ifndef uicreateattriblogdlg_h
#define uicreateattriblogdlg_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          March 2008
 RCS:           $Id: uicreateattriblogdlg.h,v 1.1 2008-03-13 11:17:48 cvssatyaki Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
#include "bufstringset.h"

namespace Attrib { class DescSet; }
class uiAttrSel;
class uiListBox;
class uiGenInput;
class uiParent;

class uiCreateAttribLogDlg : public uiDialog
{
public:
    				uiCreateAttribLogDlg(uiParent*,BufferStringSet,
					const Attrib::DescSet*);
				~uiCreateAttribLogDlg();

    const TypeSet<int>&		getSelectedItems() const;
    Notifier<uiCreateAttribLogDlg> wellselected;
    uiParent*			parent_;
protected:
    TypeSet<int>*		wellist_;
    const Attrib::DescSet*	attrib_;
    
    uiAttrSel*			attribfld_;
    uiListBox*			childlist_;
    uiGenInput*			topmrkfld_;
    uiGenInput*			botmrkfld_;
    bool			acceptOK(CallBacker*);
};

#endif
