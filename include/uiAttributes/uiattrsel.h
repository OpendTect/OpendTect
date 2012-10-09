#ifndef uiattrsel_h
#define uiattrsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiiosel.h"
#include "attribdescid.h"
#include "datapack.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; };
namespace ZDomain { class Info; }

class CubeSampling;
class IOObj;
class NLAModel;
class uiButtonGroup;
class uiGenInput;
class uiListBox;
class uiRadioButton;
class uiLabeledComboBox;


mClass uiAttrSelData
{
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

protected:

    const Attrib::DescSet*	attrset_;

};


/*! \brief UI element for selection of Attributes from an attribute set

This dialog gets an attribute ID from the set. It can be used to select
an attribute or NLA output. When it is used to select the input for
another attribute, you'll need to specify the attrib ID of that attribute as
'ignoreid'.
Because stored cubes can also be considered attributes, the user can also
select any cube, which is then automatically added to the set.

*/

mClass uiAttrSelDlg : public uiDialog
{
public:

    mClass Setup
    {
    public:
		Setup( const char* txt )
		    : seltxt_(txt)
		    , ignoreid_(Attrib::DescID::undef())
		    , isinp4otherattrib_(false)
		    , showsteeringdata_(false)
		{}

		mDefSetupMemb(BufferString,seltxt)
		mDefSetupMemb(Attrib::DescID,ignoreid)
		mDefSetupMemb(bool,isinp4otherattrib)
		mDefSetupMemb(bool,showsteeringdata)
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
    const char*		zDomainKey() const;

    bool		is2D() const		{ return attrdata_.is2D(); }
    const Attrib::DescSet& getAttrSet() const	{ return attrdata_.attrSet(); }
    int			selType() const;
    
protected:

    uiAttrSelData	attrdata_;
    Attrib::SelInfo*	attrinf_;
    bool		usedasinput_;	//input for another attribute
    bool		in_action_;
    bool		showsteerdata_;
    BufferString	zdomainkey_;

    TypeSet<DataPack::FullID> dpfids_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	steerfld_;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_;
    uiRadioButton*	zdomainfld_;

    uiListBox*		storoutfld_;
    uiListBox*		steeroutfld_;
    uiListBox*		attroutfld_;
    uiListBox*		nlaoutfld_;
    uiListBox*		zdomoutfld_;
    uiGenInput*		filtfld_;
    uiGenInput*		attr2dfld_;
    uiLabeledComboBox*	compfld_;

    void		initAndBuild(const char*,Attrib::DescID,bool);
    void		createSelectionButtons();
    void		createSelectionFields();

    bool		getAttrData(bool);
    void		replaceStoredByInMem();
    void		doFinalise( CallBacker* );
    void		selDone(CallBacker*);
    void		filtChg(CallBacker*);
    void		cubeSel(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
};


/*!\brief ui element for storing attribute desc selection.

It can be used to select an attribute or the input for an attribute. In the
latter case you must provide the attrib desc and the input number.

*/

mClass uiAttrSel : public uiIOSelect
{
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

    virtual void	getHistory(const IOPar&);
    virtual void	processInput();

    const char*		errMsg()		{ return errmsg_.str(); }
    bool		getRanges(CubeSampling&) const;
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

    TypeSet<DataPack::FullID> dpfids_;

    void		updateInput();
    void		update2D();
    void		doSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;
};


/*!\brief ui element for getting attribute with both real and imag part. */

mClass uiImagAttrSel : public uiAttrSel
{
public:
			uiImagAttrSel( uiParent* p, const char* txt,
					const uiAttrSelData& asd )
			: uiAttrSel(p,txt,asd)	{}

    inline Attrib::DescID realID() const		{ return attribID(); }
    Attrib::DescID	imagID() const;

};


#endif
