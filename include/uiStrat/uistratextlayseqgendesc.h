#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
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

    uiStratLayerModelDisp* getLayModDisp(uiStratLayModEditTools&,
				    Strat::LayerModelSuite&,int) override;
    uiObject*		outerObj() override	{ return this; }
    void		prepareDesc() override	{ getTopDepthFromScreen(); }
    void		setDescID(const MultiID&) override;
    void		setEditDesc() override;
    void		setFromEditDesc() override;
    bool		selProps() override;

protected:

    Strat::LayerSequenceGenDesc&	editdesc_;
    uiGenInput*		topdepthfld_;
    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    const uiRect	workrect_;	//!< will be filled
    bool		zinft_;		//!< From SI()
    MultiID		descid_;

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

private:
    const Strat::LayerSequenceGenDesc* editedDesc() const { return &editdesc_;}

};


#define mDefuiExtLayerSequenceGenDescFns(clss,typstr) \
    mDefuiLayerSequenceGenDescFns(clss,typstr) \
protected: \
    void		doDraw() override; \
    bool		newLayGenReq(bool) override; \
    bool		laygenEditReq() override; \
    bool		laygenRemoveReq() override; \
public: \
    void		descHasChanged() override
