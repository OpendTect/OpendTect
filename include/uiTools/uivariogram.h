#ifndef uivariogram_h
#define uivariogram_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
 RCS:		$Id: uivariogram.h,v 1.1 2011-10-18 19:25:14 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

template <class T> class Array1D;
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


mClass HorVariogramComputer
{
public:

				HorVariogramComputer(DataPointSet& dpset,
						     int step,int range,
						     int fold);
				~HorVariogramComputer();

	Array1D<float>*		getData() const;


protected:
	Array1D<float>*		variogramvals_;

	void			compVarFromRange(DataPointSet& dpset,
						 int step,int range,int fold);
};


mClass VertVariogramComputer
{
public:

				VertVariogramComputer(DataPointSet& dpset,int,
						     int step,int range,
						     int fold);
				~VertVariogramComputer();

	Array1D<float>*		getData() const;


protected:
	Array1D<float>*		variogramvals_;

	void			compVarFromRange(DataPointSet& dpset,int colid,
						 int step,int range,int fold);
};

mClass uiVariogramDisplay: public uiDialog
{
public:
     				uiVariogramDisplay(uiParent*,Array1D<float>*,
						   int maxrg,int step,
						   bool ishor);

    void			draw();
protected:
        Array1D<float>*	data_;
        uiFunctionDisplay* 	disp_;
	int			maxrg_;
	int			step_;

	uiGenInput*		typefld_;
	uiSliderExtra*		sillfld_;
	uiSliderExtra*		rangefld_;

	void			fieldChangedCB(CallBacker*);
};

#endif
