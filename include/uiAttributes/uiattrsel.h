#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "uiiosel.h"
#include "attribdescid.h"
#include "datapack.h"
#include "survgeom.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; }
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
\brief User interface for attribute selection data.
*/

mExpClass(uiAttributes) uiAttrSelData
{ mODTextTranslationClass(uiAttrSelData);
public:

				uiAttrSelData(bool is2d,bool fillwithdef=true);
				uiAttrSelData(const Attrib::DescSet&,
					      bool fillwithdef=true);

    Attrib::DescID		attribid_;
    const NLAModel*		nlamodel_;
    int				outputnr_;
    int				compnr_;
    const ZDomain::Info*	zdomaininfo_;

    bool			is2D() const;
    const Attrib::DescSet&	attrSet() const		{ return *attrset_; }
    void			setAttrSet( const Attrib::DescSet* ds )
						{ if ( ds ) attrset_ = ds; }
    void			fillSelSpec(Attrib::SelSpec&) const;

protected:

    const Attrib::DescSet*	attrset_;

};


/*!
\brief User Interface (UI) element for selection of Attributes from an
attribute set.

  This dialog gets an attribute ID from the set. It can be used to select an
  attribute or NLA output. When it is used to select the input for another
  attribute, you'll need to specify the attrib ID of that attribute as
  'ignoreid'. Because stored cubes can also be considered attributes, the user
  can also select any cube, which is then automatically added to the set.
*/

mExpClass(uiAttributes) uiAttrSelDlg : public uiDialog
{ mODTextTranslationClass(uiAttrSelDlg);
public:

    mExpClass(uiAttributes) Setup
    {
    public:
		Setup( const uiString& txt )
		    : seltxt_(txt)
		    , ignoreid_(Attrib::DescID::undef())
		    , isinp4otherattrib_(false)
		    , showsteeringdata_(false)
		    , geomid_(mUdfGeomID)
		{}

		mDefSetupMemb(uiString,seltxt)
		mDefSetupMemb(Attrib::DescID,ignoreid)
		mDefSetupMemb(bool,isinp4otherattrib)
		mDefSetupMemb(bool,showsteeringdata)
		mDefSetupMemb(Pos::GeomID,geomid)
    };

			uiAttrSelDlg(uiParent*,const uiAttrSelData&,
				     const Setup&);
			uiAttrSelDlg(uiParent*,const uiAttrSelData&,
				     const TypeSet<DataPack::FullID>&,
				     const Setup&);
			~uiAttrSelDlg();

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

protected:

    uiAttrSelData	attrdata_;
    Attrib::SelInfo*	attrinf_;
    bool		usedasinput_;	//input for another attribute
    bool		in_action_;
    bool		showsteerdata_;
    BufferString	zdomainkey_;
    Pos::GeomID		geomid_;

    TypeSet<DataPack::FullID> dpfids_;
    MultiID		insertedobjmid_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	steerfld_;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_;
    uiRadioButton*	zdomainfld_;

    uiListBox*		storoutfld_;
    uiListBox*		steeroutfld_;
    uiListBox*		attroutfld_;
    uiListBoxFilter*	attribfilterfld_	= nullptr;
    uiListBox*		nlaoutfld_;
    uiListBox*		zdomoutfld_;
    uiGenInput*		filtfld_;
    uiGenInput*		attr2dfld_;
    uiLabeledComboBox*	compfld_;

    ObjectSet<uiIOObjInserter> inserters_;
    ObjectSet<uiButton>	extselbuts_;

    void		initAndBuild(const uiString&,Attrib::DescID,bool);
    void		createSelectionButtons();
    void		createSelectionFields();

    bool		getAttrData(bool);
    void		replaceStoredByInMem();
    void		doFinalize( CallBacker* );
    void		selDone(CallBacker*);
    void		filtChg(CallBacker*);
    void		cubeSel(CallBacker*);
    void		objInserted(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};


/*!
\brief User interface element for storing attribute desc selection.

  It can be used to select an attribute or the input for an attribute. In the
  latter case you must provide the attrib desc and the input number.
*/

mExpClass(uiAttributes) uiAttrSel : public uiIOSelect
{ mODTextTranslationClass(uiAttrSel);
public:
			uiAttrSel(uiParent*,const Attrib::DescSet&,
				  const char* txt=0,
				  Attrib::DescID curid=Attrib::DescID::undef(),
				  bool isinp4otherattrib = true);
			uiAttrSel(uiParent*,const char*,const uiAttrSelData&,
				  bool isinp4otherattrib = true);
			~uiAttrSel()		{}

    Attrib::DescID	attribID() const	{ return attrdata_.attribid_; }
    int			outputNr() const	{ return attrdata_.outputnr_; }
    int			compNr() const		{ return attrdata_.compnr_; }
    inline bool		is2D() const		{ return attrdata_.is2D(); }
    inline bool		is3D() const		{ return !is2D(); }

    void		setDescSet(const Attrib::DescSet*);
			//!< This function has to be called before getHistory !
    void		setDesc(const Attrib::Desc*);
			//!< If called, it has to be called before getHistory !
			//!< If you call it, you don't need to call setDescSet.
    void		setSelSpec(const Attrib::SelSpec*);
    void		setNLAModel(const NLAModel*);

    void		setIgnoreDesc(const Attrib::Desc*);
    void		setIgnoreID( Attrib::DescID id ) { ignoreid_ = id; }
    void		setPossibleDataPacks(const TypeSet<DataPack::FullID>&);
    void		showSteeringData( bool yn )	{ showsteeringdata_=yn;}
    void		setGeomID( Pos::GeomID id )	{ geomid_ = id; }
    Pos::GeomID		getGeomID() const		{ return geomid_; }

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
    bool		is2d_;
    Attrib::DescID	ignoreid_;
    bool		usedasinput_;	//input for another attribute
    BufferString	errmsg_;
    mutable BufferString usrnm_;
    int			seltype_;
    bool		showsteeringdata_;
    Pos::GeomID		geomid_;

    TypeSet<DataPack::FullID> dpfids_;

    void		updateInput();
    void		update2D();
    void		doSel(CallBacker*);
    const char*		userNameFromKey(const char*) const override;

    static uiString	cDefLabel();
};


/*!
\brief User interface element for getting attribute with both real and
imaginary part.
*/

mExpClass(uiAttributes) uiImagAttrSel : public uiAttrSel
{ mODTextTranslationClass(uiImagAttrSel);
public:
			uiImagAttrSel( uiParent* p, const char* txt,
					const uiAttrSelData& asd )
			: uiAttrSel(p,txt,asd)	{}

    inline Attrib::DescID realID() const		{ return attribID(); }
    Attrib::DescID	imagID() const;

};
