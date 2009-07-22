#ifndef uicreateattriblogdlg_h
#define uicreateattriblogdlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2008
 RCS:           $Id: uicreateattriblogdlg.h,v 1.6 2009-07-22 16:01:24 cvsbert Exp $
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

mClass uiCreateAttribLogDlg : public uiDialog
{
public:
    				uiCreateAttribLogDlg(uiParent*,
						     const BufferStringSet&,
					             const Attrib::DescSet*,
						     const NLAModel*,bool);
				~uiCreateAttribLogDlg();
    int                         selectedLogIdx() const  { return sellogidx_; }

protected:
    const Attrib::DescSet*	attrib_;
    
    const NLAModel*		nlamodel_;
    uiAttrSel*			attribfld_;
    uiListBox*			welllistfld_;
    uiGenInput*			topmrkfld_;
    uiGenInput*			botmrkfld_;
    uiGenInput*			stepfld_;
    uiGenInput*			lognmfld_;
    const BufferStringSet&	wellnames_;
    int 			sellogidx_;
    bool 			singlewell_;

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
