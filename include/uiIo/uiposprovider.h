#ifndef uiposprovider_h
#define uiposprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovider.h,v 1.2 2008-02-07 16:10:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class IOPar;
class uiGenInput;
namespace Pos { class Provider; }
class uiPosProvGroup;

/*! \brief lets user choose a way to provide positions */

class uiPosProvider : public uiGroup
{
public:

    struct Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges };

			Setup( const char* txt, bool with_z )
			    : seltxt_(txt)
			    , withz_(with_z)
			    , is2d_(false)
			    , choicetype_(OnlyRanges)	{}
	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(ChoiceType,choicetype)
    };

    			uiPosProvider(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;

    Pos::Provider*	createProvider() const;

protected:

    uiGenInput*			selfld_;
    ObjectSet<uiPosProvGroup>	grps_;
    Setup			setup_;

    void			selChg(CallBacker*);
};


#endif
