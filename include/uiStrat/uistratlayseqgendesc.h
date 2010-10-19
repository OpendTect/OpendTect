#ifndef uistratlayseqgendesc_h
#define uistratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayseqgendesc.h,v 1.1 2010-10-19 08:52:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "factory.h"
class uiRectItem;
namespace Strat { class LayerGenerator; class LayerSequenceGenDesc; }


mClass uiLayerSequenceGenDesc : public uiGraphicsView
{
public:

    				uiLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
	    			Strat::LayerSequenceGenDesc&,factory);

protected:

    Strat::LayerSequenceGenDesc& desc_;
    uiRectItem*		outeritm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled

    void		reDraw(CallBacker*);

    virtual void	doDraw()			= 0;

    Strat::LayerGenerator* curgen_;
    virtual bool	newDescReq()			= 0;
    virtual bool	descEditReq()			= 0;
    virtual bool	descRemoveReq()			= 0;

};


#define mDefuiLayerSequenceGenDescFns(clss,typstr) \
    static const char*	typeStr()			{ return typstr; } \
    virtual const char*	type() const			{ return typeStr(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
					   Strat::LayerSequenceGenDesc& gd ) \
						    { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
protected: \
    virtual void	doDraw(); \
    virtual bool	newDescReq(); \
    virtual bool	descEditReq(); \
    virtual bool	descRemoveReq()
    

#endif
