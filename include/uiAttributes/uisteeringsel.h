#ifndef uisteeringsel_h
#define uisteeringsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiseissel.h"
#include "uiattrsel.h"
#include "attribdescid.h"

class uiGenInput;
class uiLabel;
namespace Attrib { class Desc; class DescSet; class SelSpec; };


mExpClass(uiAttributes) uiSteerCubeSel : public uiSeisSel
{
public:

				uiSteerCubeSel(uiParent*,CtxtIOObj&,
				       const Attrib::DescSet*,bool,
				       const char* txt="Steering Data" );

    inline Attrib::DescID	inlDipID() const	{ return getDipID(0); }
				// Returns -2 when selected is not a dip
    inline Attrib::DescID 	crlDipID() const	{ return getDipID(1); }
				// Returns -2 when selected is not a dip

    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);

    void			fillSelSpec(Attrib::SelSpec&,bool inl);
    				/* inl=true: AttribSelSpec for inline comp
    				   inl=false: AttribSelSpec for crossline comp*/

    static const IOObjContext&	ioContext(bool is2d);
    static CtxtIOObj*		mkCtxtIOObj(bool is2d,bool forread);

protected:

    Attrib::DescID		getDipID(int) const;
    void			doFinalise(CallBacker*);

    uiAttrSelData		attrdata_;
};



/*!\brief Attribute Steering ui element: data + selection of type. */

mExpClass(uiAttributes) uiSteeringSel : public uiGroup
{
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

    static IOPar&		inpselhist;

protected:

    const Attrib::DescSet*	descset_;
    CtxtIOObj&			ctio_;

    uiLabel*			nosteerlbl_;
    uiGenInput*			typfld_;
    uiGenInput*			dirfld_;
    uiGenInput*			dipfld_;
    uiSteerCubeSel*		inpfld_;

    bool			is2d_;
    bool			notypechange_;
    bool			withconstdir_;

    void			createFields();
    void			doFinalise(CallBacker*);
    virtual void		typeSel(CallBacker*);
};

#endif

