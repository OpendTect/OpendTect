#ifndef uivariogram_h
#define uivariogram_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
 RCS:		$Id: uivariogram.h,v 1.5 2012/03/02 13:43:30 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

template <class T> class Array2D;
class BufferStringSet;
class DataPointSet;
class uiGenInput;
class uiFunctionDisplay;
class uiSliderExtra;
class uiSpinBox;

mClass uiVariogramDlg : public uiDialog
{
public:

				uiVariogramDlg(uiParent*,bool);

    int				getMaxRg() const;
    int				getStep() const;
    int				getFold() const;

protected:

    uiSpinBox*			maxrgfld_;
    uiSpinBox*			stepfld_;
    uiSpinBox*			foldfld_;
    void			stepChgCB(CallBacker*);

};


mClass uiVariogramDisplay: public uiDialog
{
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
        uiFunctionDisplay* 	disp_;
	int			maxrg_;

	uiGenInput*		labelfld_;
	uiGenInput*		typefld_;
	uiSliderExtra*		sillfld_;
	uiSliderExtra*		rangefld_;

	void			labelChangedCB(CallBacker*);
	void			fieldChangedCB(CallBacker*);
};

#endif
