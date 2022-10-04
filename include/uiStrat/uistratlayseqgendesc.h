#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "factory.h"

class PropertyRefSelection;

class uiParent;
class uiObject;
class uiStratLayerModelDisp;
class uiStratLayModEditTools;
namespace Strat { class LayerSequenceGenDesc; class LayerModelSuite; }


/*!\brief Base class for LayerSequenceGenDesc editors - with factory.

  A uiLayerSequenceGenDesc immediately has to produce a layer model displayer,
  which may be itself.

  The subclasses have to keep track whether anything has changed. If so,
  needSave() will return true. You can force the flag with setNeedSave().

 */

mExpClass(uiStrat) uiLayerSequenceGenDesc
{ mODTextTranslationClass(uiLayerSequenceGenDesc);
public:

			uiLayerSequenceGenDesc(Strat::LayerSequenceGenDesc&);
    virtual		~uiLayerSequenceGenDesc();
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
			Strat::LayerSequenceGenDesc&,factory);

    virtual bool	separateDisplay()		{ return true; }

    virtual void	descHasChanged()		= 0;
    virtual uiObject*	outerObj()			= 0;
    virtual uiStratLayerModelDisp* getLayModDisp(uiStratLayModEditTools&,
					Strat::LayerModelSuite&,int opt=0) = 0;

    virtual void	prepareDesc()			{}
    virtual void	setEditDesc()			{}
    virtual void	setFromEditDesc()		{}
    Strat::LayerSequenceGenDesc& desc()			{ return desc_; }
    const Strat::LayerSequenceGenDesc& currentDesc() const;
    bool		needSave() const		{ return needsave_; }
    void		setNeedSave( bool yn )		{ needsave_ = yn; }
    virtual void	setDescID(const MultiID&)	{}
    virtual bool	selProps();
    virtual void	setDispProp(int propidx)	{}

protected:

    Strat::LayerSequenceGenDesc& desc_;
    bool		needsave_ = false;
    bool		isValidSelection(const PropertyRefSelection&) const;
    virtual const uiParent* getUiParent() const		= 0;

private:
    const Strat::LayerSequenceGenDesc* editedDesc() const
			{ return nullptr; }

};



#define mDefuiLayerSequenceGenDescFns(clss,typstr) \
public: \
    static const char*	typeStr()			{ return typstr; } \
    const char*		factoryKeyword() const override { return typeStr(); } \
    const uiParent*	getUiParent() const override	{ return parent(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
					   Strat::LayerSequenceGenDesc& gd ) \
						    { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr()); }
