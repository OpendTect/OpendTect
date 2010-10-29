#ifndef uistratlayseqgendesc_h
#define uistratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayseqgendesc.h,v 1.6 2010-10-29 09:08:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "property.h"
class uiRectItem;
class uiTextItem;
namespace Strat { class LayerSequenceGenDesc; }


mClass uiLayerSequenceGenDesc : public uiGraphicsView
{
public:

    				uiLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
	    			Strat::LayerSequenceGenDesc&,factory);

    Strat::LayerSequenceGenDesc& desc()		{ return desc_; }
    virtual void	getPropertyRefs(PropertyRefSelection&) const	= 0;

    virtual void	descHasChanged()	= 0;

protected:

    Strat::LayerSequenceGenDesc& desc_;
    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled

    void		reDraw(CallBacker*);
    void		usrClickCB(CallBacker*);

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
    virtual const char*	type() const			{ return typeStr(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
					   Strat::LayerSequenceGenDesc& gd ) \
						    { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual void	descHasChanged()
    

#endif
