#ifndef uisteersel_h
#define uisteersel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uisteeringsel.h,v 1.10 2007-06-07 05:29:22 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uiattrsel.h"
#include "attribdescid.h"

namespace Attrib { class Desc; class DescSet; class SelSpec; };

class ChangeTracker;
class uiGenInput;
class uiLabel;

class uiSteerCubeSel : public uiSeisSel
{
public:

			uiSteerCubeSel(uiParent*,CtxtIOObj&,
				       const Attrib::DescSet*,bool,
				       const char* txt="Steering Data" );

    inline Attrib::DescID inlDipID() const	{ return getDipID(0); }
			// Returns -2 when selected is not a dip
    inline Attrib::DescID crlDipID() const	{ return getDipID(1); }
			// Returns -2 when selected is not a dip

    void		setDesc(const Attrib::Desc*);
    void		setDescSet(const Attrib::DescSet*);

    void		fillSelSpec(Attrib::SelSpec&,bool inl);
    			/* inl=true: AttribSelSpec for inline component
    			   inl=false: AttribSelSpec for crossline component */

protected:

    uiAttrSelData	attrdata;

    Attrib::DescID	getDipID(int) const;
    CtxtIOObj&		getCtio(CtxtIOObj&);

    void		doFinalise(CallBacker*);
};



/*!\brief Attribute Steering ui element: data + selection of type. */

class uiSteeringSel : public uiGroup
{
public:
				uiSteeringSel(uiParent*,
					      const Attrib::DescSet*,bool is2d,
					      bool withconstdir=true);
				~uiSteeringSel();

    Attrib::DescID		descID();

    bool			willSteer() const;
    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);
    void			setType(int,bool fixed=false);

    static IOPar&		inpselhist;

protected:

    const Attrib::DescSet*	descset_;
    CtxtIOObj&			ctio_;

    uiLabel*			nosteerlbl_;
    uiGenInput*			typfld;
    uiSteerCubeSel*		inpfld;
    uiGenInput*			dirfld;
    uiGenInput*			dipfld;

    bool			is2d_;
    bool			notypechange_;

    void			doFinalise(CallBacker*);
    void			typeSel(CallBacker*);
};

#endif
