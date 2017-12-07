#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2017
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

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(uiString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiSeisPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSeisSel*		seissel_;
    uiSelZRange*	zrgfld_;

    static const char*	sKeyType() { return Pos::SeisProvider3D::sKeyType(); }

};
