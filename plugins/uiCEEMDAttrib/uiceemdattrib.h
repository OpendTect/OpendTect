#ifndef uiceemdattrib_h
#define uiceemdattrib_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/

#include "gendefs.h"
#include "uiattrdesced.h"
#include "attribdescid.h"
#include "uiattribpanel.h"

namespace Attrib { class Desc; class DescSet; }
class uiAttrSel;
class uiCEEMDPanel;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiSpecDecompPanel;
class uiTrcPositionDlg;

class uiCEEMDAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiCEEMDAttrib);
public:

    uiCEEMDAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		methodfld_;
    uiGenInput*		maximffld_;
    uiGenInput*		stopimffld_;
    uiGenInput*		maxsiftfld_;
    uiGenInput*		stopsiftfld_;
    uiGenInput*		outputfreqfld_;
    uiGenInput*		stepoutfreqfld_;
    uiGenInput*		attriboutputfld_;
    uiGenInput*		outputcompfld_;

    uiPushButton*	tfpanelbut_;
    uiCEEMDPanel*	panelview_;	//!< Time Frequency panel
    uiTrcPositionDlg*	positiondlg_;
    IOPar		prevpar_;

    void		panelTFPush(CallBacker*);
    void		outSel(CallBacker*);
    void		getInputDBKey(DBKey&) const;
    void		setPrevSel();
    void		getPrevSel();
    void		viewPanelCB(CallBacker*);
    Attrib::DescID	createCEEMDDesc(Attrib::DescSet*) const;
    Attrib::Desc*	createNewDesc(Attrib::DescSet*,Attrib::DescID,
				      const char*,int inpidx,
				      BufferString) const;
    void		fillInCEEMDDescParams(Attrib::Desc*) const;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    mDeclReqAttribUIFns
};


class uiCEEMDPanel	: public uiAttribPanel
{
public:
				uiCEEMDPanel( uiParent* p )
				    : uiAttribPanel( p )		{};

protected:
    virtual const char*		getProcName();
    virtual const char*		getPackName();
    virtual const char*		getPanelName();

};

#endif
