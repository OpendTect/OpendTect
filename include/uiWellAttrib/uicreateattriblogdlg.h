#ifndef uicreateattriblogdlg_h
#define uicreateattriblogdlg_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          March 2008
 RCS:           $Id: uicreateattriblogdlg.h,v 1.2 2008-04-10 05:24:11 cvssatyaki Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
#include "binidvalset.h"
#include "bufstringset.h"

namespace Attrib { class DescSet; }
namespace Well { class Data; }
class NLAModel;
class uiAttrSel;
class uiListBox;
class uiGenInput;

class uiCreateAttribLogDlg : public uiDialog
{
public:
    				uiCreateAttribLogDlg(uiParent*,
						     const BufferStringSet&,
					             const Attrib::DescSet*,
						     const NLAModel*);
				~uiCreateAttribLogDlg();

protected:
    const Attrib::DescSet*	attrib_;
    
    const NLAModel*		nlamodel_;
    uiAttrSel*			attribfld_;
    uiListBox*			welllistfld_;
    uiGenInput*			topmrkfld_;
    uiGenInput*			botmrkfld_;
    uiGenInput*			stepfld_;
    uiGenInput*			lognmfld_;
    BufferStringSet		markernames_;
    int 			sellogidx_;

    bool                        inputsOK(int);
    bool                        getPositions(BinIDValueSet&,Well::Data&,
					     TypeSet<BinIDValueSet::Pos>&,
					     TypeSet<float>& depths);
    bool                        extractData(BinIDValueSet&);
    bool                        createLog(const BinIDValueSet&,Well::Data&,
					  const TypeSet<BinIDValueSet::Pos>&,
					  const TypeSet<float>& depths);
    bool			acceptOK(CallBacker*);
    void			selDone(CallBacker*);
};

#endif
