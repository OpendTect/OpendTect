#ifndef uigapdeconattrib_h
#define uigapdeconattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July 2006
 RCS:           $Id: uigapdeconattrib.h,v 1.12 2006-10-10 17:46:05 cvsbert Exp $
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
class GapDeconACorrView;


/*! \brief GapDecon Attribute description editor */

class uiGapDeconAttrib : public uiAttrDescEd
{
public:

			uiGapDeconAttrib(uiParent*);
			~uiGapDeconAttrib();

    void                set2D(bool);
    void		getEvalParams(TypeSet<EvalParam>&) const;

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

    			mDeclReqAttribUIFns
};


#endif
