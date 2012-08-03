#ifndef uistratextlayseqgendesc_h
#define uistratextlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratextlayseqgendesc.h,v 1.2 2012-08-03 13:01:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratlayseqgendesc.h"
#include "uigraphicsview.h"
#include "property.h"
class uiRectItem;
class uiTextItem;
namespace Strat { class LayerSequenceGenDesc; }


/*!\brief Base class for external LayerSequenceGenDesc editors, i.e.
  editors that are not also Layer Model displayers. */

mClass(uiStrat) uiExtLayerSequenceGenDesc : public uiGraphicsView
				 , public uiLayerSequenceGenDesc
{
public:

    				uiExtLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);

    virtual uiObject*			outerObj()	{ return this; }
    virtual uiStratLayerModelDisp*	getLayModDisp(uiStratLayModEditTools&,
	    				Strat::LayerModel&);

protected:

    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled

    void		reDraw(CallBacker*);
    void		singClckCB( CallBacker* cb )	{ hndlClick(cb,false); }
    void		dblClckCB( CallBacker* cb )	{ hndlClick(cb,true); }
    void		hndlClick(CallBacker*,bool);

    virtual void	doDraw()			= 0;

    uiPoint		clickpos_;
    virtual bool	newLayGenReq(bool above)	= 0;
    virtual bool	laygenEditReq()			= 0;
    virtual bool	laygenRemoveReq()		= 0;

};


#define mDefuiExtLayerSequenceGenDescFns(clss,typstr) \
    mDefuiLayerSequenceGenDescFns(clss,typstr) \
protected: \
    virtual void	doDraw(); \
    virtual bool	newLayGenReq(bool); \
    virtual bool	laygenEditReq(); \
    virtual bool	laygenRemoveReq(); \
public: \
    virtual void	descHasChanged()
    

#endif

