#ifndef uispecdecompattrib_h
#define uispecdecompattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2003
 RCS:           $Id: uispecdecompattrib.h,v 1.12 2011/06/13 06:10:07 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "uiattrdesced.h"
#include "uiattribpanel.h"
#include "iopar.h"

namespace Attrib { class Desc; };

class uiGenInput;
class uiImagAttrSel;
class uiLabeledSpinBox;
class uiPushButton;
class uiSpecDecompPanel;
class uiTrcPositionDlg;
class BinID;

/*! \brief Spectral Decomposition Attribute description editor */

mClass uiSpecDecompAttrib : public uiAttrDescEd
{
public:

			uiSpecDecompAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;
    int			getOutputIdx(float) const;
    float		getOutputValue(int) const;

protected:

    uiImagAttrSel*	inpfld_;
    uiGenInput*		typefld_;
    uiGenInput*         gatefld_;
    uiLabeledSpinBox*	outpfld_;
    uiLabeledSpinBox*	stepfld_;
    uiGenInput*		waveletfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		inputSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		stepChg(CallBacker*);
    void		panelTFPush(CallBacker*);

    void		checkOutValSnapped() const;
    void		getInputMID(MultiID&) const;
    Attrib::DescID	createSpecDecompDesc(Attrib::DescSet*) const;
    void		createHilbertDesc(Attrib::DescSet*,
	    				  Attrib::DescID&) const;
    Attrib::Desc*	createNewDesc(Attrib::DescSet*,Attrib::DescID,
	    			      const char*,int,int,BufferString) const;
    void		fillInSDDescParams(Attrib::Desc*) const;
    bool		passStdCheck(const Attrib::Desc*,const char*,
	    			     int seloutidx,int inpidx,
				     Attrib::DescID inpid) const;
    void		viewPanalCB(CallBacker*);
    void		setPrevSel();
    void		getPrevSel();
    static const char* sKeyBinID();
    static const char* sKeyLineName();
    static const char* sKeyTrcNr();

    float		nyqfreq_;
    int			nrsamples_; //!< Nr of samples in selected data
    float		ds_; //!< Sample spacing of selected data

    uiPushButton*	tfpanelbut_;
    uiSpecDecompPanel*	panelview_;	//!< Time Frequency panel
    uiTrcPositionDlg*	positiondlg_;
    IOPar		prevpar_;

    			mDeclReqAttribUIFns
};


class uiSpecDecompPanel	: public uiAttribPanel
{
public:
    				uiSpecDecompPanel( uiParent* p )
				    : uiAttribPanel( p )		{};

protected:
    virtual const char*         getProcName();
    virtual const char*         getPackName();
    virtual const char*         getPanelName();

};

#endif
