#ifndef uistratlayseqgendesc_h
#define uistratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayseqgendesc.h,v 1.9 2010-12-21 15:01:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "property.h"
class uiRectItem;
class uiTextItem;
namespace Strat { class LayerSequenceGenDesc; }


/*!\brief Base class for LayerSequenceGenDesc editors - with factory.
 
  The subclasses have to keep track whether anything has changed. If so,
  needSave() will return true. You can force the flag with setNeedSave().
 
 */

mClass uiLayerSequenceGenDesc : public uiGraphicsView
{
public:

    				uiLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
	    			Strat::LayerSequenceGenDesc&,factory);

    Strat::LayerSequenceGenDesc& desc()		{ return desc_; }
    virtual void	getPropertyRefs(PropertyRefSelection&) const	= 0;
    bool		needSave() const	{ return needsave_; }
    void		setNeedSave( bool yn )	{ needsave_ = yn; }

    virtual void	descHasChanged()	= 0;

protected:

    Strat::LayerSequenceGenDesc& desc_;
    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled
    bool		needsave_;

    void		reDraw(CallBacker*);
    void		singClckCB( CallBacker* cb )	{ hndlClick(cb,false); }
    void		dblClckCB( CallBacker* cb )	{ hndlClick(cb,true); }
    void		hndlClick(CallBacker*,bool);

    virtual void	doDraw()			= 0;

    uiPoint		clickpos_;
    virtual bool	newDescReq(bool above)		= 0;
    virtual bool	descEditReq()			= 0;
    virtual bool	descRemoveReq()			= 0;

};


#define mDefuiLayerSequenceGenDescFns(clss,typstr) \
protected: \
    virtual void	doDraw(); \
    virtual bool	newDescReq(bool); \
    virtual bool	descEditReq(); \
    virtual bool	descRemoveReq(); \
public: \
    static const char*	typeStr()			{ return typstr; } \
    virtual const char*	factoryKeyword() const		{ return typeStr(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
					   Strat::LayerSequenceGenDesc& gd ) \
						    { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual void	descHasChanged()
    

#endif
