#ifndef uisteersel_h
#define uisteersel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uisteeringsel.h,v 1.5 2005-08-19 07:17:53 cvshelene Exp $
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
				       const Attrib::DescSet*,
				       const char* txt="Steering Data" );

    inline Attrib::DescID inlDipID() const	{ return getDipID(0); }
			// Returns -2 when selected is not a dip
    inline Attrib::DescID crlDipID() const	{ return getDipID(1); }
			// Returns -2 when selected is not a dip

    void		setDesc(const Attrib::Desc*);
    void		setDescSet(const Attrib::DescSet*);

    void		updateAttrSet2D(); // If attrset is changed but

    void		fillSelSpec(Attrib::SelSpec&,bool inl);
    			/* inl=true: AttribSelSpec for inline component
    			   inl=false: AttribSelSpec for crossline component */

protected:

    uiAttrSelData	attrdata;

    Attrib::DescID	getDipID(int) const;
    void		update2D();
    CtxtIOObj&		getCtio(CtxtIOObj&);

};



/*!\brief Attribute Steering ui element: data + selection of type. */

class uiSteeringSel : public uiGroup
{
public:
				uiSteeringSel(uiParent*,const Attrib::DescSet*);
				~uiSteeringSel();

    Attrib::DescID		descID();

    bool			willSteer() const;
    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);
    void			setType(int,bool fixed=false);
    void			set2D(bool);

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
    bool			fixed_;

    void			doFinalise(CallBacker*);
    void			typeSel(CallBacker*);
};

#endif
