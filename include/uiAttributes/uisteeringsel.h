#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiseissel.h"
#include "uiattrsel.h"
#include "attribdescid.h"

class uiGenInput;
class uiLabel;
namespace Attrib { class Desc; class DescSet; class SelSpec; }

mExpClass(uiAttributes) uiSteerAttrSel : public uiSteerCubeSel
{ mODTextTranslationClass(uiSteerAttrSel);
public:
				uiSteerAttrSel(uiParent*,
					      const Attrib::DescSet*,bool is2d,
					      const uiString& txt=
					      mJoinUiStrs(sSteering(),sData()));
				~uiSteerAttrSel();

    inline Attrib::DescID	inlDipID() const	{ return getDipID(0); }
				// Returns -2 when selected is not a dip
    inline Attrib::DescID	crlDipID() const	{ return getDipID(1); }
				// Returns -2 when selected is not a dip

    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);

    void			fillSelSpec(Attrib::SelSpec&,bool inl);
				/* inl=true: AttribSelSpec for inline comp
				   inl=false: AttribSelSpec for crossline comp*/

protected:

    Attrib::DescID		getDipID(int) const;

    uiAttrSelData		attrdata_;
};



/*!\brief Attribute Steering ui element: data + selection of type. */

mExpClass(uiAttributes) uiSteeringSel : public uiGroup
{ mODTextTranslationClass(uiSteeringSel);
public:
				uiSteeringSel(uiParent*,
					      const Attrib::DescSet*,bool is2d,
					      bool withconstdir=true,
					      bool doinit=true);
				~uiSteeringSel();

    Attrib::DescID		descID();

    virtual bool		willSteer() const;
    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);
    void			setType(int,bool fixed=false);

    void			clearInpField();
    const char*			text() const;

    Notifier<uiSteeringSel>	steertypeSelected_;

protected:

    const Attrib::DescSet*	descset_;

    uiLabel*			nosteerlbl_;
    uiGenInput*			typfld_;
    uiGenInput*			dirfld_;
    uiGenInput*			dipfld_;
    uiSteerAttrSel*		inpfld_;

    bool			is2d_;
    bool			notypechange_;
    bool			withconstdir_;

    void			createFields();
    void			doFinalize(CallBacker*);
    virtual void		typeSel(CallBacker*);
};
