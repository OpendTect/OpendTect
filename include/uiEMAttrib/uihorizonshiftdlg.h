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
#include "bufstringset.h"
#include "datapack.h"
#include "emposid.h"
#include "position.h"

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
			uiHorizonShiftDialog(uiParent*,const EM::ObjectID& id,
					     VisID visid,
					     const Attrib::DescSet&,
					     float initialshift,
					     bool cancalcattrib);
			~uiHorizonShiftDialog();

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
    EM::Horizon*		horizon_		= nullptr;
    EM::ObjectID		emid_;
    VisID			visid_;
};

