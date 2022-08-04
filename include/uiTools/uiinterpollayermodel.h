#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "factory.h"
#include "uigroup.h"
#include "uistrings.h"

class uiGenInput;
class InterpolationLayerModel;


mExpClass(uiTools) uiInterpolationLayerModelGrp : public uiGroup
{
public:
			mDefineFactory1ParamInClass(
				uiInterpolationLayerModelGrp,
				uiParent*,factory)

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			uiInterpolationLayerModelGrp(uiParent*);

};


mExpClass(uiTools) uiZSliceInterpolationModel
			: public uiInterpolationLayerModelGrp
{ mODTextTranslationClass(uiZSliceInterpolationModel)
public:
			mDefaultFactoryInstantiation1Param(
				uiInterpolationLayerModelGrp,
				uiZSliceInterpolationModel,uiParent*,
			       "ZSlices",uiStrings::sZSlice(mPlural))

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			uiZSliceInterpolationModel(uiParent*);
};


mExpClass(uiTools) uiInterpolationLayerModel : public uiGroup
{ mODTextTranslationClass(uiInterpolationLayerModel)
public:
			uiInterpolationLayerModel(uiParent*);

    InterpolationLayerModel* getModel();
    void		setModel(const InterpolationLayerModel*);

protected:
    void		selCB(CallBacker*);

    uiGenInput*					layermodelfld_;
    ObjectSet<uiInterpolationLayerModelGrp>	grps_;
};

