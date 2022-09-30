#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

			~uiInterpolationLayerModelGrp();

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

			~uiZSliceInterpolationModel();

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			uiZSliceInterpolationModel(uiParent*);
};


mExpClass(uiTools) uiInterpolationLayerModel : public uiGroup
{ mODTextTranslationClass(uiInterpolationLayerModel)
public:
			uiInterpolationLayerModel(uiParent*);
			~uiInterpolationLayerModel();

    InterpolationLayerModel* getModel();
    void		setModel(const InterpolationLayerModel*);

protected:
    void		selCB(CallBacker*);

    uiGenInput*					layermodelfld_;
    ObjectSet<uiInterpolationLayerModelGrp>	grps_;
};
