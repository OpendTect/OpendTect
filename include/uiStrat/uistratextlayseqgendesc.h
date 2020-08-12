#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratlayseqgendesc.h"
#include "uigraphicsview.h"
#include "property.h"
class uiGenInput;
class uiRectItem;
class uiTextItem;
namespace Strat { class LayerSequenceGenDesc; }


/*!\brief Base class for external LayerSequenceGenDesc editors, i.e.
  editors that are not also Layer Model displayers. */

mExpClass(uiStrat) uiExtLayerSequenceGenDesc : public uiGraphicsView
				 , public uiLayerSequenceGenDesc
{ mODTextTranslationClass(uiExtLayerSequenceGenDesc)
public:

    				uiExtLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);

    virtual uiObject*			outerObj()	{ return this; }
    virtual uiStratLayerModelDisp*	getLayModDisp(uiStratLayModEditTools&,
					    Strat::LayerModelProvider&,int);
    virtual void	prepareDesc()	{ getTopDepthFromScreen(); }
    virtual void	setEditDesc();
    virtual void	setFromEditDesc();
    virtual bool	selProps();

    const Strat::LayerSequenceGenDesc& editedDesc() const { return editdesc_; }

protected:

    Strat::LayerSequenceGenDesc&	editdesc_;
    uiGenInput*		topdepthfld_;
    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled
    bool		zinft_;		//!< From SI()

    void		getTopDepthFromScreen();
    void		putTopDepthToScreen();
    void		reDraw(CallBacker*);
    void		wheelMoveCB(CallBacker*);
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
    

