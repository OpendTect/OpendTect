#ifndef uiinterpollayermodel_h
#define uiinterpollayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "factory.h"
#include "uigroup.h"

class uiGenInput;
class InterpolationLayerModel;


mExpClass(uiTools) uiInterpolationLayerModelGrp : public uiGroup
{
public:
			mDefineFactory1ParamInClass(
				uiInterpolationLayerModelGrp,
				uiParent*,factory)

    virtual bool	fillPar(IOPar&) const;

protected:
			uiInterpolationLayerModelGrp(uiParent*);

};


mExpClass(uiTools) uiZSliceInterpolationModel
			: public uiInterpolationLayerModelGrp
{
public:
			mDefaultFactoryInstantiation1Param(
				uiInterpolationLayerModelGrp,
				uiZSliceInterpolationModel,uiParent*,
				"ZSlices","Z Slices")

    bool		fillPar(IOPar&) const;

protected:
			uiZSliceInterpolationModel(uiParent*);
};


mExpClass(uiTools) uiInterpolationLayerModel : public uiGroup
{
public:
			uiInterpolationLayerModel(uiParent*);

    InterpolationLayerModel* getModel();

protected:
    void		selCB(CallBacker*);

    uiGenInput*					layermodelfld_;
    ObjectSet<uiInterpolationLayerModelGrp>	grps_;
};

#endif
