#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"
#include "attribdescid.h"
#include "emposid.h"

class NLAModel;
class uiAttrSel;
class uiCheckBox;
class uiGenInput;
class uiSlider;
class uiPushButton;
namespace EM { class Horizon; }
namespace Attrib { class DescSet; }

mClass(uiEMAttrib) uiHorizonShiftDialog : public uiDialog
{ mODTextTranslationClass(uiHorizonShiftDialog);
public:
			uiHorizonShiftDialog(uiParent*,const EM::ObjectID&,
					     const VisID&,
					     const Attrib::DescSet&,
					     float initialshift,
					     bool cancalcattrib);
			~uiHorizonShiftDialog();

    void			setDescSet(const Attrib::DescSet*);
    void			setNLAModel(const NLAModel*);

    StepInterval<float>		shiftRg() const;
    int				nrSteps() const;
    Attrib::DescID		attribID() const;
    float			getShift() const;
    int				curShiftIdx() const;
    bool			doStore() const;
    const EM::ObjectID&		emID() const		{ return emid_; }
    VisID			visID() const		{ return visid_; }

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
    bool			acceptOK(CallBacker*) override;

    uiAttrSel*			attrinpfld_		= nullptr;
    uiGenInput*			rangeinpfld_;
    uiSlider*			slider_;
    uiPushButton*		calbut_			= nullptr;
    uiCheckBox*			storefld_		= nullptr;
    uiGenInput*			namefld_		= nullptr;

    StepInterval<float>		shiftrg_;
    StepInterval<float>		calcshiftrg_;
    RefMan<EM::Horizon>		horizon_;
    EM::ObjectID		emid_;
    VisID			visid_;
};
