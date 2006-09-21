#ifndef uigapdeconattrib_h
#define uigapdeconattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July 2006
 RCS:           $Id: uigapdeconattrib.h,v 1.5 2006-09-21 15:15:29 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "attribdescid.h"
#include "position.h"
#include "ranges.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;
class uiGDPositionDlg;

using namespace Attrib;

/*! \brief GapDecon Attribute description editor */

class uiGapDeconAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiGapDeconAttrib(uiParent*);

    void		getEvalParams(TypeSet<EvalParam>&) const;
    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		lagfld_;
    uiGenInput*		gapfld_;
    uiGenInput*		noiselvlfld_;
    uiGenInput*		isinpzerophasefld_;
    uiGenInput*		isoutzerophasefld_;
    uiLabeledSpinBox*	nrtrcsfld_;
    uiPushButton*	exambut_;

    uiGDPositionDlg*	positiondlg_;
    bool		setParameters(const Desc&);
    bool		setInput(const Desc&);

    bool		getParameters(Desc&);
    bool		getInput(Desc&);

    void                examPush(CallBacker*);
    bool		passStdCheck(const Desc*,const char*,int,int,DescID);
    bool		passVolStatsCheck(const Desc*,BinID,Interval<float>);
    Desc* 		createNewDesc(DescSet*,DescID,const char*,
	    			      int,int,BufferString);
    DescID		createVolStatsDesc(Desc&,int);
    void		createHilbertDesc(Desc&,DescID&);
    DescID		createInvHilbertDesc(Desc&);
};


#endif
