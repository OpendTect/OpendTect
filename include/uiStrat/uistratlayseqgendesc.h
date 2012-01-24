#ifndef uistratlayseqgendesc_h
#define uistratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayseqgendesc.h,v 1.14 2012-01-24 16:40:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "factory.h"
class uiComboBox;
class uiStratLayerModelDisp;
class uiStratLayModEditTools;
namespace Strat { class Content; class RefTree; }
namespace Strat { class LayerSequenceGenDesc; class LayerModel; }


/*!\brief Base class for LayerSequenceGenDesc editors - with factory.

  A uiLayerSequenceGenDesc immediately has to produce a layer model displayer,
  which may be itself.
 
  The subclasses have to keep track whether anything has changed. If so,
  needSave() will return true. You can force the flag with setNeedSave().
 
 */

mClass uiLayerSequenceGenDesc
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
				    Strat::LayerModel&)	= 0;

    Strat::LayerSequenceGenDesc& desc()			{ return desc_; }
    bool		needSave() const		{ return needsave_; }
    void		setNeedSave( bool yn )		{ needsave_ = yn; }
    void		selProps();

protected:

    Strat::LayerSequenceGenDesc& desc_;
    bool		needsave_;

};


mClass uiStratLayerContent : public uiGroup
{
public:

  			uiStratLayerContent(uiParent*,
					    const Strat::RefTree* rt=0);

    void		set(const Strat::Content&);
    const Strat::Content& get() const;

protected:

    uiComboBox*		fld_;
    const Strat::RefTree& rt_;
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
