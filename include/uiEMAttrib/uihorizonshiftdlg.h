#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/

#include "uidialog.h"
#include "attribdescid.h"
#include "datapack.h"
#include "position.h"

class uiAttrSel;
class uiCheckBox;
class uiGenInput;
class uiSlider;
class uiPushButton;
namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; }

mClass(uiEMAttrib) uiHorizonShiftDialog : public uiDialog
{ mODTextTranslationClass(uiHorizonShiftDialog);
public:
			uiHorizonShiftDialog(uiParent*,const DBKey& id,
					     int visid,
					     const Attrib::DescSet&,
					     float initialshift,
					     bool cancalcattrib);
			~uiHorizonShiftDialog();

    const EM::Horizon3D&	horizon3D()		{ return *emhor3d_; }
    StepInterval<float>	shiftRg() const;
    int				nrSteps() const;
    Attrib::DescID		attribID() const;
    float			getShift() const;
    int				curShiftIdx() const;
    bool			doStore() const;
    const DBKey&		emID() const		{ return emid_; }
    const int&			visID() const		{ return visid_; }

    const char*			getAttribName() const;
    const char*			getAttribBaseName() const;

    Notifier<uiHorizonShiftDialog>	calcAttribPushed;
    Notifier<uiHorizonShiftDialog>	horShifted;

protected:
    static const char*		sDefaultAttribName();

    void			setNameFldSensitive(CallBacker*);
    void			rangeChangeCB(CallBacker*);
    void			attribChangeCB(CallBacker*);
    void			calcAttrib(CallBacker*);
    void			shiftCB(CallBacker*);
    bool			acceptOK();

    uiAttrSel*			attrinpfld_;
    uiGenInput*			rangeinpfld_;
    uiSlider*			slider_;
    uiPushButton*		calbut_;
    uiCheckBox*			storefld_;
    uiGenInput*			namefld_;

    StepInterval<float>		shiftrg_;
    StepInterval<float>		calcshiftrg_;
    EM::Horizon3D*		emhor3d_;
    DBKey			emid_;
    int				visid_;
};
