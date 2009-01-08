#ifndef uiattrsel_h
#define uiattrsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrsel.h,v 1.13 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiiosel.h"
#include "attribdescid.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; };

class CubeSampling;
class IOObj;
class NLAModel;
class uiButtonGroup;
class uiGenInput;
class uiListBox;
class uiRadioButton;
class uiLabeledSpinBox;


mClass uiAttrSelData
{
public:
				uiAttrSelData( const Attrib::DescSet* a )
				: attrset(a), attribid(-1,true)
				, nlamodel(0), outputnr(-1), compnr(-1)
				, shwcubes(true)	{}

    const Attrib::DescSet*	attrset;
    Attrib::DescID		attribid;
    const NLAModel*		nlamodel;
    int				outputnr;
    int				compnr;
    bool			shwcubes;
    BufferString		zdomainkey;

    const Attrib::DescSet&	attrSet() const	{ return *attrset; }
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

			uiAttrSelDlg(uiParent*,const char* seltxt,
				     const uiAttrSelData&,bool is2d=false,
				     Attrib::DescID ignoreid=
				     		Attrib::DescID(-1,true));
			~uiAttrSelDlg();

			// if ( go() ) ...
    Attrib::DescID	attribID() const	{ return attrdata_.attribid; }
			//!< -1 if not selected
    int			outputNr() const	{ return attrdata_.outputnr; }
			//!< -1 if not selected
    const char*		zDomainKey() const	{ return zdomainkey_; }

protected:

    uiAttrSelData	attrdata_;
    Attrib::SelInfo*	attrinf_;
    bool		in_action_;
    BufferString	zdomainkey_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_;
    uiRadioButton*	zdomainfld_;

    uiListBox*		storoutfld_;
    uiListBox*		attroutfld_;
    uiListBox*		nlaoutfld_;
    uiListBox*		zdomoutfld_;
    uiGenInput*		filtfld_;
    uiGenInput*		attr2dfld_;
    uiLabeledSpinBox*	compfld_;

    void		createSelectionButtons();
    void		createSelectionFields();

    bool		getAttrData(bool);
    void		doFinalise( CallBacker* );
    void		selDone(CallBacker*);
    void		filtChg(CallBacker*);
    void		cubeSel(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    int			selType() const;
};


/*!\brief ui element for storing attribute desc selection.


It can be used to select an attribute or the input for an attribute. In the
latter case you must provide the attrib desc and the input number.

*/


mClass uiAttrSel : public uiIOSelect
{
public:
			uiAttrSel(uiParent*,const Attrib::DescSet*,bool is2d,
				  const char* txt=0,
				  Attrib::DescID curid=Attrib::DescID::undef());
			uiAttrSel(uiParent*,const char*,
				  const uiAttrSelData&,bool is2d);
			~uiAttrSel()		{}

    Attrib::DescID	attribID() const	{ return attrdata_.attribid; }
    int			outputNr() const	{ return attrdata_.outputnr; }
    bool		is2D() const		{ return is2d_; }
    bool		is3D() const		{ return !is2d_; }

    void		setDescSet(const Attrib::DescSet*);
			//!< This function has to be called before getHistory !
    void		setDesc(const Attrib::Desc*);
			//!< If called, it has to be called before getHistory !
			//!< If you call it, you don't need to call setDescSet.
    void		setNLAModel(const NLAModel*);

    void		setIgnoreDesc(const Attrib::Desc*);
    void		setIgnoreID( Attrib::DescID id ) { ignoreid = id; }

    virtual void	getHistory(const IOPar&);
    virtual void	processInput();

    const char*		errMsg()		{ return errmsg; }
    bool		getRanges(CubeSampling&) const;
    			//!< Tries to determine ranges of currently selected.

    void		fillSelSpec(Attrib::SelSpec&) const;
    bool		checkOutput(const IOObj&) const;
    const char*		getAttrName() const;

    void		setObjectName(const char*);
    
protected:

    uiAttrSelData	attrdata_;
    bool		is2d_;
    Attrib::DescID	ignoreid;
    BufferString	errmsg;
    mutable BufferString usrnm;

    void		updateInput();
    void		update2D();
    void		doSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;
};


/*!\brief ui element for getting attribute with both real and imag part. */

mClass uiImagAttrSel : public uiAttrSel
{
public:
			uiImagAttrSel(uiParent* p,const Attrib::DescSet* a,
				      bool is2d, const char* txt=0,
				      Attrib::DescID curid=
				      		Attrib::DescID(-1,true))
			: uiAttrSel(p,a,is2d,txt,curid)	{}

    inline Attrib::DescID realID() const		{ return attribID(); }
    Attrib::DescID	imagID() const;

};


#endif
