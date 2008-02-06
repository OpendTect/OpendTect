#ifndef uiposprovider_h
#define uiposprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovider.h,v 1.1 2008-02-06 16:04:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "factory.h"
class IOPar;
class uiGenInput;
namespace Pos { class Provider; }

/*! \brief group for subselecting an area for 2D or 3D seismics */

class uiPosProvGroup : public uiGroup
{
public:
			uiPosProvGroup(uiParent*);

    virtual void	usePar(const IOPar&)		= 0;
    virtual bool	fillPar(IOPar&) const		= 0;

    mDefineFactory1ParamInClass(uiPosProvGroup,uiParent*,factory);

};

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
