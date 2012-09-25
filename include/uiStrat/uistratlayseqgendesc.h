#ifndef uistratlayseqgendesc_h
#define uistratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "factory.h"
class uiParent;
class uiObject;
class uiStratLayerModelDisp;
class uiStratLayModEditTools;
namespace Strat { class LayerSequenceGenDesc; class LayerModelProvider; }


/*!\brief Base class for LayerSequenceGenDesc editors - with factory.

  A uiLayerSequenceGenDesc immediately has to produce a layer model displayer,
  which may be itself.
 
  The subclasses have to keep track whether anything has changed. If so,
  needSave() will return true. You can force the flag with setNeedSave().
 
 */

mClass(uiStrat) uiLayerSequenceGenDesc
{
public:

  			uiLayerSequenceGenDesc(Strat::LayerSequenceGenDesc&);
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
	    		Strat::LayerSequenceGenDesc&,factory);
    virtual		~uiLayerSequenceGenDesc()	{}
    virtual bool	separateDisplay()		{ return true; }

    virtual void	descHasChanged()		= 0;
    virtual uiObject*	outerObj()			= 0;
    virtual uiStratLayerModelDisp* getLayModDisp(uiStratLayModEditTools&,
				    Strat::LayerModelProvider&)	= 0;

    Strat::LayerSequenceGenDesc& desc()			{ return desc_; }
    bool		needSave() const		{ return needsave_; }
    void		setNeedSave( bool yn )		{ needsave_ = yn; }
    bool		selProps();

protected:

    Strat::LayerSequenceGenDesc& desc_;
    bool		needsave_;

};



#define mDefuiLayerSequenceGenDescFns(clss,typstr) \
public: \
    static const char*	typeStr()			{ return typstr; } \
    virtual const char*	factoryKeyword() const		{ return typeStr(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
					   Strat::LayerSequenceGenDesc& gd ) \
						    { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr()); }


#endif

