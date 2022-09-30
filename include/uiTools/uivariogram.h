#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uistring.h"

template <class T> class Array2D;
class BufferStringSet;
class uiGenInput;
class uiFunctionDisplay;
class uiSlider;
class uiSpinBox;

mExpClass(uiTools) uiVariogramDlg : public uiDialog
{ mODTextTranslationClass(uiVariogramDlg);
public:
				uiVariogramDlg(uiParent*,bool);
				~uiVariogramDlg();

    int				getMaxRg() const;
    int				getStep() const;
    int				getFold() const;

protected:

    uiSpinBox*			maxrgfld_;
    uiSpinBox*			stepfld_;
    uiSpinBox*			foldfld_;
    void			stepChgCB(CallBacker*);

};


mExpClass(uiTools) uiVariogramDisplay : public uiDialog
{ mODTextTranslationClass(uiVariogramDisplay);
public:
				uiVariogramDisplay(uiParent*,Array2D<float>*,
						   Array2D<float>*,
						   BufferStringSet*,
						   int maxrg,
						   bool ishor);
				~uiVariogramDisplay();

    void			draw();

protected:
        Array2D<float>*		data_;
	Array2D<float>*		axes_;
	BufferStringSet*	labels_;
        uiFunctionDisplay*	disp_;
	int			maxrg_;

	uiGenInput*		labelfld_;
	uiGenInput*		typefld_;
	uiSlider*		sillfld_;
	uiSlider*		rangefld_;

	void			labelChangedCB(CallBacker*);
	void			fieldChangedCB(CallBacker*);
};
