#ifndef uicreateattriblogdlg_h
#define uicreateattriblogdlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2008
 RCS:           $Id$
 _______________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"
#include "createattriblog.h"

namespace Attrib { class DescSet; }
namespace Well { class Data; }
class NLAModel;
class uiAttrSel;
class uiListBox;
class uiGenInput;
class uiWellExtractParams;

mClass uiCreateAttribLogDlg : public uiDialog
{
public:
    				uiCreateAttribLogDlg(uiParent*,
						     const BufferStringSet&,
					             const Attrib::DescSet*,
						     const NLAModel*,bool);
				~uiCreateAttribLogDlg(){};
    int                         selectedLogIdx() const  
    				{ return sellogidx_; }

protected:
    
    uiAttrSel*			attribfld_;
    uiListBox*			welllistfld_;
    uiGenInput*			lognmfld_;
    uiWellExtractParams*	zrangeselfld_;
    const BufferStringSet&	wellnames_;
    int 			sellogidx_;
    bool 			singlewell_;
    AttribLogCreator::Setup	datasetup_;

    bool                        inputsOK(int);
    bool			acceptOK(CallBacker*);
    void			selDone(CallBacker*);

    void			init(CallBacker*);
};

#endif
