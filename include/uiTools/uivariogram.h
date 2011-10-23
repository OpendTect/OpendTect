#ifndef uivariogram_h
#define uivariogram_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
 RCS:		$Id: uivariogram.h,v 1.2 2011-10-23 12:40:06 cvshelene Exp $
________________________________________________________________________

-*/

#include "arrayndutils.h"
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

	bool			compVarFromRange(DataPointSet& dpset,
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

	bool			isOK() const		{ return dataisok_; }


protected:
	Array1D<float>*		variogramvals_;

	bool			dataisok_;

	bool			compVarFromRange(DataPointSet& dpset,int colid,
						 int step,int range,int fold);

	struct MDandRowID
	{
	    		MDandRowID( double md=0, int rowid=0 )
			    : md_(md)
			    , rowid_(rowid)	{};

	    double	md_;
	    int		rowid_;

	    bool	operator>( MDandRowID challenger ) const
				{ return md_ > challenger.md_; }
	    bool	operator==( MDandRowID challenger ) const
				{ return md_ == challenger.md_; }
	};
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
