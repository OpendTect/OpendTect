#ifndef uivariogram_h
#define uivariogram_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
 RCS:		$Id: uivariogram.h,v 1.3 2012-02-26 17:47:32 cvshelene Exp $
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


mClass HorVariogramComputer
{
public:

				HorVariogramComputer(DataPointSet& dpset,
						     int step,int range,
						     int fold);
				~HorVariogramComputer();

	Array2D<float>*		getData() const;
	BufferStringSet*	getLabels() const;

	bool			isOK() const		{ return dataisok_; }

protected:
	Array2D<float>*		variogramvals_;
	BufferStringSet*	variogramnms_;

	bool			dataisok_;

	bool			compVarFromRange(DataPointSet& dpset,
						 int step,int range,int fold);
};


mClass VertVariogramComputer
{
public:

				VertVariogramComputer(DataPointSet& dpset,int,
						     int step,int range,
						     int fold, int nrgroups);
				~VertVariogramComputer();

	Array2D<float>*		getData() const;
	Array2D<float>*		getStd() const;
	Array2D<od_int64>*	getFold() const;
	BufferStringSet*	getLabels() const;

	bool			isOK() const		{ return dataisok_; }

protected:
	Array2D<float>*		variogramvals_;
	Array2D<float>*		variogramstds_;
	Array2D<od_int64>*	variogramfolds_;
	BufferStringSet*	variogramnms_;

	bool			dataisok_;

	bool			compVarFromRange(DataPointSet& dpset,int colid,
						 int step,int range,int fold,
						 int nrgroups);

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
     				uiVariogramDisplay(uiParent*,Array2D<float>*,
						   BufferStringSet*,
						   int maxrg,int step,
						   bool ishor);

				~uiVariogramDisplay();

    void			draw();

protected:
        Array2D<float>*		data_;
	BufferStringSet*	labels_;
        uiFunctionDisplay* 	disp_;
	int			maxrg_;
	int			step_;

	uiGenInput*		labelfld_;
	uiGenInput*		typefld_;
	uiSliderExtra*		sillfld_;
	uiSliderExtra*		rangefld_;

	void			labelChangedCB(CallBacker*);
	void			fieldChangedCB(CallBacker*);
};

#endif
