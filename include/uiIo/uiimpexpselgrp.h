#ifndef uiimpexpselgrp_h
#define uiimpexpselgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Dec 2010
 RCS:           $Id: uiimpexpselgrp.h,v 1.1 2010-12-02 10:04:34 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uidatapointsetcrossplot.h"
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiAxisHandler;
class uiComboBox;
class uiDataPointSetCrossPlotter;

mClass uiReadSelGrp : public uiDialog
{
public:
			uiReadSelGrp(uiParent*,uiDataPointSetCrossPlotter&);

protected:

    uiFileInput*	inpfld_;
    uiComboBox*		xselfld_;
    uiComboBox*		yselfld_;
    uiComboBox*		y2selfld_;
    uiCheckBox*		ychkfld_;
    uiCheckBox*		y2chkfld_;
    uiDataPointSetCrossPlotter&	plotter_;
		
    ObjectSet<SelectionGrp>& selgrpset_;
    BufferString	xname_;
    BufferString	yname_;
    BufferString	y2name_;

    bool		adjustSelectionGrps();
    bool		checkSelectionArea(SelectionArea&,
	    				   const BufferStringSet& selaxisnms,
	    				   const BufferStringSet& avlblaxnms,
					   bool);

    void		fillRectangle(const SelectionArea& selarea,
	    			      SelectionArea& actselarea);
    void		fillPolygon(const SelectionArea& selarea,
	    			    SelectionArea& actselarea);

    BufferStringSet	getAvailableAxisNames() const;
    void		getInfo(const ObjectSet<SelectionGrp>&,BufferString&);
    void		fldCheckedCB(CallBacker*);
    void		examineCB(CallBacker*);
    bool		acceptOK(CallBacker*);
};


mClass uiExpSelectionArea : public uiDialog
{
public:

	mClass Setup
	{
	    public:
		    Setup(const char* x,const char* y, const char* y2)
			: y2name_(y2) , yname_(y) , xname_(x) {}

	    mDefSetupMemb(const char*,y2name)
	    mDefSetupMemb(const char*,yname)
	    mDefSetupMemb(const char*,xname)
	};
	    
					uiExpSelectionArea(uiParent*,
						const ObjectSet<SelectionGrp>&,
						uiExpSelectionArea::Setup);
	    

protected:

    Setup				setup_;
    uiGenInput*				axisfld_;
    uiFileInput*			outfld_;
    const ObjectSet<SelectionGrp>&	selgrps_;

    bool				acceptOK(CallBacker*);

};

#endif
