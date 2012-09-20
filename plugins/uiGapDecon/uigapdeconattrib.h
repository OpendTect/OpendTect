#ifndef uigapdeconattrib_h
#define uigapdeconattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigapdeconmod.h"
#include "uiattrdesced.h"
#include "attribdescid.h"
#include "position.h"
#include "ranges.h"

class MultiID;
class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;
class uiGDPositionDlg;
class GapDeconACorrView;


/*! \brief GapDecon Attribute description editor */

class uiGapDeconAttrib : public uiAttrDescEd
{
public:

			uiGapDeconAttrib(uiParent*,bool);
			~uiGapDeconAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const;
    static const char*	sKeyOnInlineYN();
    static const char*	sKeyLineName();

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		lagfld_;
    uiGenInput*		gapfld_;
    uiGenInput*		noiselvlfld_;
    uiGenInput*		isinpzerophasefld_;
    uiGenInput*		isoutzerophasefld_;
    uiGenInput*		wantmixfld_;
    uiLabeledSpinBox*	stepoutfld_;
    uiPushButton*	exambut_;
    uiPushButton*	qcbut_;

    IOPar		par_;
    uiGDPositionDlg*	positiondlg_;
    GapDeconACorrView*	acorrview_;
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void                examPush(CallBacker*);
    void                qCPush(CallBacker*);
    void                mixSel(CallBacker*);
    bool		passStdCheck(const Attrib::Desc*,const char*,int,int,
	    			     Attrib::DescID);
    bool		passVolStatsCheck(const Attrib::Desc*,BinID,
	    				  Interval<float>);
    Attrib::Desc* 	createNewDesc(Attrib::DescSet*,Attrib::DescID,
	    			      const char*,int,int,BufferString);
    Attrib::DescID	createVolStatsDesc(Attrib::Desc&,int);
    void		createHilbertDesc(Attrib::Desc&,Attrib::DescID&);
    Attrib::DescID	createGapDeconDesc(Attrib::DescID&,Attrib::DescID,
	    				   DescSet*,bool);
    void		prepareInputDescs(Attrib::DescID&,Attrib::DescID&,
	    				  Attrib::DescSet*);
    void		fillInGDDescParams(Attrib::Desc*);
    void		getInputMID(MultiID&) const;

    			mDeclReqAttribUIFns
};


#endif
