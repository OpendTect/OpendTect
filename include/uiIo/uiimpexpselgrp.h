#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uidatapointsetcrossplot.h"
#include "uigroup.h"
#include "filepath.h"

class BufferStringSet;

class uiAxisHandler;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiGenInput;
class uiPushButton;
class uiToolButton;
class uiDataPointSetCrossPlotter;
class od_istream;
class od_ostream;


mExpClass(uiIo) uiSGSel : public uiGroup
{ mODTextTranslationClass(uiSGSel);
public:
					uiSGSel(uiParent*,bool forread);

    bool				isOK() const;
    const char*				selGrpFileNm();
    const char*				selGrpSetNm() const;
    const ObjectSet<SelectionGrp>&	selGrpSet() const
					{ return selgrpset_; }

    const OD::String&			xName()		{ return xname_; }
    const OD::String&			yName()		{ return yname_; }
    const OD::String&			y2Name()	{ return y2name_; }

    Notifier<uiSGSel>			selGrpSelected;

protected:

    uiGenInput*			inpfld_;
    uiPushButton*		selbut_;

    BufferString		selgrpfilenm_;
    BufferString		xname_;
    BufferString		yname_;
    BufferString		y2name_;

    ObjectSet<SelectionGrp>	selgrpset_;
    bool			forread_;
    void			selectSGCB(CallBacker*);
};


mExpClass(uiIo) uiSGSelGrp : public uiGroup
{ mODTextTranslationClass(uiSGSelGrp);
public:
				uiSGSelGrp(uiParent*,bool forread);

    BufferString		getCurFileNm() const;
    const char*			selGrpSetNm() const;
    bool			getCurSelGrpSet(ObjectSet<SelectionGrp>&);

    const OD::String&		xName()			{ return xname_; }
    const OD::String&		yName()			{ return yname_; }
    const OD::String&		y2Name()		{ return y2name_; }

    Notifier<uiSGSelGrp>	selectionDone;

protected:

    uiListBox*			listfld_;
    uiGenInput*			nmfld_;
    uiToolButton*		infobut_;
    uiToolButton*		delbut_;
    uiToolButton*		renamebut_;

    BufferString		xname_;
    BufferString		yname_;
    BufferString		y2name_;
    bool			forread_;

    FilePath			basefp_;

    bool			createBaseDir();
    bool			hasIdxFile();
    bool			fillListBox();
    bool			getSelGrpSetNames(BufferStringSet&) const;
    bool			setSelGrpSetNames(const BufferStringSet&) const;

    void			showInfo(CallBacker*);
    void			delSelGrps(CallBacker*);
    void			renameSelGrps(CallBacker*);
    void			selChangedCB(CallBacker*);
    void			selDoneCB(CallBacker*);
};


mExpClass(uiIo) SelGrpImporter
{ mODTextTranslationClass(uiIo);
public:
				SelGrpImporter(const char*);
				~SelGrpImporter();

    ObjectSet<SelectionGrp>	getSelections();
    const uiString&		errMsg()		{ return errmsg_; }
    const OD::String&		xName()			{ return xname_; }
    const OD::String&		yName()			{ return yname_; }
    const OD::String&		y2Name()		{ return y2name_; }

protected:

    uiString			errmsg_;
    BufferString		xname_;
    BufferString		yname_;
    BufferString		y2name_;

    od_istream*			strm_		= nullptr;

};


mExpClass(uiIo) SelGrpExporter
{ mODTextTranslationClass(SelGrpExporter);
public:
				SelGrpExporter(const char* fnm);
				~SelGrpExporter();
    bool			putSelections(const ObjectSet<SelectionGrp>&,
					      const char* xnm,
					      const char* ynm,
					      const char* y2nm);
    const uiString&		errMsg() const		{ return errmsg_; }

protected:

    uiString			errmsg_;
    od_ostream*			strm_		= nullptr;

};


mExpClass(uiIo) uiReadSelGrp : public uiDialog
{ mODTextTranslationClass(uiReadSelGrp);
public:
			uiReadSelGrp(uiParent*,uiDataPointSetCrossPlotter&);

protected:

    uiSGSel*		inpfld_;
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
    void		selectedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};


mExpClass(uiIo) uiExpSelectionArea : public uiDialog
{ mODTextTranslationClass(uiExpSelectionArea);
public:

	mExpClass(uiIo) Setup
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
    uiSGSel*				outfld_;
    const ObjectSet<SelectionGrp>&	selgrps_;

    bool				acceptOK(CallBacker*) override;

};
