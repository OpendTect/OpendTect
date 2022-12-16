#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiposprovgroup.h"
#include "seisposprovider.h"
class uiSeisSel;
class uiSelZRange;


/*! \brief UI for SeisPosProvider */

mExpClass(uiSeis) uiSeisPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiSeisPosProvGroup);
public:
			uiSeisPosProvGroup(uiParent*,
					    const uiPosProvGroup::Setup&);
			~uiSeisPosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiSeisPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSeisSel*		seissel_	    = nullptr;
    uiSelZRange*	zrgfld_		    = nullptr;

    static const char*	sKeyType() { return Pos::SeisProvider3D::sKeyType(); }

};
