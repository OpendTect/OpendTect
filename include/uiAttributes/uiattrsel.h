#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "attribdescid.h"
#include "uidlggroup.h"
#include "uiiosel.h"
#include "datapack.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; }
namespace ZDomain { class Info; }

class IOObj;
class NLAModel;
class TrcKeyZSampling;
class uiGenInput;
class uiIOObjInserter;
class uiListBox;
class uiLabeledComboBox;
class uiRadioButton;
class uiAttrDescEd;
class uiAttrSelWorkData;


/*!\brief holds a selection for an attribute selector. */

mExpClass(uiAttributes) uiAttrSelData : public CallBacker
{ mODTextTranslationClass(uiAttrSelData);
public:

    typedef Attrib::Desc	Desc;
    typedef Attrib::DescID	DescID;
    typedef Attrib::DescSet	DescSet;
    typedef Attrib::SelSpec	SelSpec;

				uiAttrSelData(bool is2d);
				uiAttrSelData(const DescSet&);
				~uiAttrSelData();

    bool			isUndef() const;
    void			setUndef();

    DescID			attribid_;
    const NLAModel*		nlamodel_;
    const ZDomain::Info*	zdomaininfo_;

    bool			is2D() const;
    bool			isNLA() const;
    const DescSet&		attrSet() const		{ return *attrset_; }
    void			setAttrSet(const DescSet&);
    void			setOutputNr( int nr )	{ nr_ = nr; }
    void			setCompNr( int nr )	{ nr_ = nr; }
    int				outputNr() const	{ return nr_; };
    int				compNr() const		{ return nr_; };

    void			fillSelSpec(SelSpec&) const;

protected:

    const DescSet*		attrset_;
    int				nr_;

    void			descSetDel(CallBacker*);

private:

    void			init();

};


/*!\brief base class for selectors of stored data/attribute/NLA output node. */

mExpClass(uiAttributes) uiAttrSelectionObj
{
public:

    enum SelType	{ Stored, Steer, Attrib, NLA };

    typedef Attrib::Desc			Desc;
    typedef Attrib::DescID			DescID;
    typedef Attrib::DescSet			DescSet;
    typedef Attrib::SelSpec			SelSpec;
    typedef DataPack::FullID			DPID;
    typedef TypeSet<DPID>			DPIDSet;

    bool		is2D() const		{ return seldata_.is2D(); }
    inline bool		is3D() const		{ return !is2D(); }
    const DescSet&	attrSet() const		{ return seldata_.attrSet(); }

    DescID		ignoreID() const	{ return ignoreid_; }
    const DPIDSet&	dataPackIDs() const	{ return dpids_; }
    const char*		zDomainKey() const;
    virtual SelType	selType() const		= 0;

    DescID		attribID() const	{ return seldata_.attribid_; }
    int			outputNr() const	{ return seldata_.outputNr(); }
    int			compNr() const		{ return seldata_.compNr(); }

    void		fillSelSpec(SelSpec&) const;

    static BufferString selTypeIconID(SelType);

protected:

			uiAttrSelectionObj(const uiAttrSelData&,bool showsteer);
    virtual		~uiAttrSelectionObj();

    uiAttrSelData	seldata_;
    Attrib::SelInfo*	attrinf_;
    DPIDSet		dpids_;
    bool		showsteerdata_;
    DescID		ignoreid_;

    uiString		selTypeDispStr(SelType) const;
    bool		have(SelType) const;
    void		fillAttrInf();

};


/*!\brief dialog for selection of stored data/attribute/NLA output node. */

mExpClass(uiAttributes) uiAttrSelDlg : public uiDialog
				     , public uiAttrSelectionObj
{ mODTextTranslationClass(uiAttrSelDlg)
public:

    mExpClass(uiAttributes) Setup
    {
    public:
		Setup( const uiString& txt )
		    : seltxt_(txt)
		    , showsteeringdata_(false)		{}

		mDefSetupMemb(uiString,seltxt)
		mDefSetupMemb(DescID,ignoreid)
		mDefSetupMemb(bool,showsteeringdata)
    };

			uiAttrSelDlg(uiParent*,const uiAttrSelData&,
				     const Setup&);
			uiAttrSelDlg(uiParent*,const uiAttrSelData&,
				     const TypeSet<DataPack::FullID>&,
				     const Setup&);
			~uiAttrSelDlg();

    virtual SelType	selType() const;

protected:

    DBKey		insertedobjdbky_;
    bool		fully_finalised_;

    uiRadioButton*	storedtypsel_;
    uiRadioButton*	steertypsel_;
    uiRadioButton*	attribtypsel_;
    uiRadioButton*	nlatypsel_;
    ObjectSet<uiRadioButton> typsels_;

    uiListBox*		storedentriesfld_;
    uiListBox*		steerentriesfld_;
    uiListBox*		attribentriesfld_;
    uiListBox*		nlaentriesfld_;
    ObjectSet<uiListBox> entriesflds_;
    uiListBox*		entryList4Type(SelType);

    uiGenInput*		filtfld_;
    uiLabeledComboBox*	compfld_;

    ObjectSet<uiIOObjInserter> inserters_;
    ObjectSet<uiButton>	insbuts_;

    void		initAndBuild(const Setup&);
    uiGroup*		createSelectionButtons();
    uiGroup*		createSelectionFields();

    bool		getAttrData(bool);
    void		finaliseWinCB( CallBacker* );
    void		selDoneCB(CallBacker*);
    void		filtChgCB(CallBacker*);
    void		cubeSelCB(CallBacker*);
    void		objInsertedCB(CallBacker*);
    virtual bool	acceptOK();
};


/*!\brief single-line selector for stored data/attribute/NLA output node. */

mExpClass(uiAttributes) uiAttrSel : public uiGroup
				  , public uiAttrSelectionObj
{ mODTextTranslationClass(uiAttrSel);
public:

			uiAttrSel(uiParent*,const DescSet&,
				  const uiString& txt=sDefLabel(),
				  DescID curid=DescID());
			uiAttrSel(uiParent*,
				  const uiAttrSelData&,
				  const uiString& txt=sDefLabel());
			~uiAttrSel();

    const DescSet&	attrSet() const		{ return seldata_.attrSet(); }
    void		setDesc(const Desc*);
    void		setDescSet(const DescSet*);
    void		setSelSpec(const SelSpec*);
    void		setNLAModel(const NLAModel*);

    void		setIgnoreDesc(const Desc*);
    void		setDataPackInputs(const DPIDSet&);
    void		showSteeringData(bool);

    bool		getRanges(TrcKeyZSampling&) const;
			//!< Tries to determine ranges of currently selected.
    bool		isValidOutput(const IOObj&) const;

    virtual SelType	selType() const;
    bool		haveSelection() const;
    const char*		getAttrName() const;

    static uiString	sDefLabel(); // uiStrings::sInputData()
    static uiString	sQuantityToOutput();

    Notifier<uiAttrSel> selectionChanged;

protected:

    const uiString	lbltxt_;
    uiComboBox*		typfld_;
    uiComboBox*		selfld_;

    void		createFields();
    void		fillTypFld();
    void		fillSelFld();
    void		addTypeFldItem(SelType);
    void		addCBsToDescSet();
    void		removeCBsFromDescSet();
    void		switchToDescSet(const DescSet&);
    void		updateContent(bool getnewinf=false,bool updtypes=false);
    void		getSelectionFromScreen();
    void		putSelectionToScreen();
    uiAttrDescEd*	getParentADE();

    void		initFlds(CallBacker*);
    void		doSelCB(CallBacker*);
    void		typSelCB(CallBacker*);
    void		selChgCB(CallBacker*);
    void		descSetChgCB(CallBacker*);
    void		adeDescSetChgCB(CallBacker*);

};


/*!\brief uiAttrsel for getting attribute with both real and imaginary part */

mExpClass(uiAttributes) uiImagAttrSel : public uiAttrSel
{ mODTextTranslationClass(uiImagAttrSel);
public:
			uiImagAttrSel( uiParent* p, const uiString& txt,
					const uiAttrSelData& asd )
			    : uiAttrSel(p,asd,txt)	{}

    inline DescID	realID() const			{ return attribID(); }
    DescID		imagID() const;

};
