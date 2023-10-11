#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattrsel.h"

#include "uidialog.h"
#include "uiiosel.h"

#include "attribdescid.h"
#include "attribsel.h"
#include "survgeom.h"

namespace Attrib
{
class Desc;
class DescSet;
class SelInfo;
class SelSpec;
}
namespace ZDomain { class Info; }

class IOObj;
class NLAModel;
class TrcKeyZSampling;
class uiButtonGroup;
class uiGenInput;
class uiIOObjInserter;
class uiLabeledComboBox;
class uiListBox;
class uiListBoxFilter;
class uiRadioButton;

/*!
\brief User Interface (UI) element for selection of Attributes from an
attribute set.

Almost a clone of uiAttrSelDlg. This class is to enable HorizonData selection.
*/

mExpClass(uiEMAttrib) uiEMAttrSelDlg : public uiDialog
{
mODTextTranslationClass(uiEMAttrSelDlg)
public:

    mExpClass(uiEMAttrib) Setup
    {
    public:
		Setup( const uiString& txt )
		    : seltxt_(txt)
		    , ignoreid_(Attrib::DescID::undef())
		    , isinp4otherattrib_(false)
		    , showsteeringdata_(false)
		    , geomid_(mUdfGeomID)
		{}
		~Setup()
		{}

		mDefSetupMemb(uiString,seltxt)
		mDefSetupMemb(Attrib::DescID,ignoreid)
		mDefSetupMemb(bool,isinp4otherattrib)
		mDefSetupMemb(bool,showsteeringdata)
		mDefSetupMemb(Pos::GeomID,geomid)
    };

			uiEMAttrSelDlg(uiParent*,const uiAttrSelData&,
				       const MultiID& horid,const Setup&);
			~uiEMAttrSelDlg();

			// if ( go() ) ...
    Attrib::DescID	attribID() const	{ return attrdata_.attribid_; }
			//!< -1 if not selected
    int			outputNr() const	{ return attrdata_.outputnr_; }
			//!< -1 if not selected
    int			compNr() const		{ return attrdata_.compnr_; }
			//!< -1 if not selected
    const char*		zDomainKey() const;

    bool		is2D() const		{ return attrdata_.is2D(); }
    const Attrib::DescSet& getAttrSet() const	{ return attrdata_.attrSet(); }
    int			selType() const;
    void		fillSelSpec(Attrib::SelSpec&) const;
    BufferString	getHorizonDataName() const;

protected:

    uiAttrSelData	attrdata_;
    Attrib::SelInfo*	attrinf_;
    bool		usedasinput_;	//input for another attribute
    bool		in_action_				= false;
    bool		showsteerdata_;
    BufferString	zdomainkey_;

    MultiID		insertedobjmid_;
    const MultiID	hormid_;

    uiButtonGroup*	selgrp_					= nullptr;
    uiRadioButton*	storbut_				= nullptr;
    uiRadioButton*	steerbut_				= nullptr;
    uiRadioButton*	attrbut_				= nullptr;
    uiRadioButton*	nlabut_					= nullptr;
    uiRadioButton*	zdomainbut_				= nullptr;
    uiRadioButton*	hordatabut_				= nullptr;

    uiListBox*		storoutfld_				= nullptr;
    uiListBox*		steeroutfld_				= nullptr;
    uiListBox*		attroutfld_				= nullptr;
    uiListBoxFilter*	attribfilterfld_			= nullptr;
    uiListBox*		nlaoutfld_				= nullptr;
    uiListBox*		zdomoutfld_				= nullptr;
    uiListBox*		hordatafld_				= nullptr;

    uiGenInput*		filtfld_				= nullptr;
    uiGenInput*		attr2dfld_				= nullptr;
    uiLabeledComboBox*	compfld_				= nullptr;

    ObjectSet<uiIOObjInserter> inserters_;
    ObjectSet<uiButton> extselbuts_;

    void		initAndBuild(const uiString&,Attrib::DescID);
    void		createSelectionButtons();
    void		createSelectionFields();

    bool		getAttrData(bool);
    void		doFinalize( CallBacker* );
    void		selDone(CallBacker*);
    void		filtChg(CallBacker*);
    void		cubeSel(CallBacker*);
    void		objInserted(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};


/*!
\brief User interface element for storing attribute desc selection.

  Almost a clone of uiAttrSel. This class is to enable HorizonData selection.
*/

mExpClass(uiEMAttrib) uiEMAttrSel : public uiIOSelect
{
mODTextTranslationClass(uiEMAttrSel)
public:
			uiEMAttrSel(uiParent*,const MultiID& hormid,
				    const Attrib::DescSet&,
				    const uiString& lbl={},
				    Attrib::DescID curid=
					Attrib::SelSpec::cAttribNotSel(),
				    bool isinp4otherattrib=true);
			~uiEMAttrSel();

    Attrib::DescID	attribID() const	{ return attrdata_.attribid_; }
    int			outputNr() const	{ return attrdata_.outputnr_; }
    int			compNr() const		{ return attrdata_.compnr_; }
    inline bool		is2D() const		{ return attrdata_.is2D(); }
    inline bool		is3D() const		{ return !is2D(); }

    void		setDescSet(const Attrib::DescSet*);
			//!< This function has to be called before getHistory !
    void		setSelSpec(const Attrib::SelSpec*);
    void		setNLAModel(const NLAModel*);

    void		setIgnoreDesc(const Attrib::Desc*);
    void		setIgnoreID( Attrib::DescID id ) { ignoreid_ = id; }
    void		showSteeringData( bool yn )	{ showsteeringdata_=yn;}

    void		getHistory(const IOPar&) override;
    void		processInput() override;

    const char*		errMsg()		{ return errmsg_.str(); }
    bool		getRanges(TrcKeyZSampling&) const;
			//!< Tries to determine ranges of currently selected.

    void		fillSelSpec(Attrib::SelSpec&) const;
    bool		checkOutput(const IOObj&) const;
    const char*		getAttrName() const;

    void		setObjectName(const char*);
    const Attrib::DescSet& getAttrSet() const	{ return attrdata_.attrSet(); }

protected:

    uiAttrSelData	attrdata_;
    BufferString	horizondataname_;
    Attrib::DescID	ignoreid_;
    bool		usedasinput_;	//input for another attribute
    BufferString	errmsg_;
    mutable BufferString usrnm_;
    int			seltype_				= -1;
    bool		showsteeringdata_			= false;
    const MultiID	hormid_;

    void		updateInput();
    void		doSel(CallBacker*);
    const char*		userNameFromKey(const char*) const override;

    static uiString	cDefLabel();
};


mExpClass(uiEMAttrib) uiEMRGBAttrSelDlg : public uiDialog
{
mODTextTranslationClass(uiEMRGBAttrSelDlg)
public:
			uiEMRGBAttrSelDlg(uiParent*,const MultiID& hormid,
					  const Attrib::DescSet&);
			~uiEMRGBAttrSelDlg();

    void		setSelSpec(const TypeSet<Attrib::SelSpec>&);
    void		fillSelSpec(TypeSet<Attrib::SelSpec>&) const;

protected:

    bool		acceptOK(CallBacker*) override;

    uiEMAttrSel*	rfld_;
    uiEMAttrSel*	gfld_;
    uiEMAttrSel*	bfld_;
    uiEMAttrSel*	tfld_;

};
