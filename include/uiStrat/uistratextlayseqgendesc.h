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
#include "uigroup.h"
#include "property.h"
#include "dbkey.h"
class uiGenInput;
class uiGraphicsView;
class uiGraphicsScene;
class uiLineEdit;
class uiRectItem;
class uiTextItem;
namespace Strat { class LayerSequenceGenDesc; }


/*!\brief Base class for external LayerSequenceGenDesc editors, i.e.
  editors that are not also Layer Model displayers. */

mExpClass(uiStrat) uiExtLayerSequenceGenDesc : public uiGroup
					     , public uiLayerSequenceGenDesc
{ mODTextTranslationClass(uiExtLayerSequenceGenDesc)
public:

				uiExtLayerSequenceGenDesc(uiParent*,
					    LayerSequenceGenDesc&);

    virtual uiStratLayerModelDisp* getLayModDisp(uiStratLayModEditTools&,
					    Strat::LayerModelSuite&,int);
    virtual uiObject*	outerObj()	{ return &asUiObject(); }
    virtual void	prepareDesc()	{ setDescStartDepth(); }
    virtual void	setDescID(const DBKey&);
    virtual void	setEditDesc();
    virtual void	setFromEditDesc();
    virtual bool	selProps();

protected:

    LayerSequenceGenDesc& editdesc_;
    uiGraphicsView*	gv_;
    uiGenInput*		topdepthfld_;
    uiLineEdit*		nmfld_;
    uiRectItem*		outeritm_;
    uiTextItem*		emptyitm_;
    uiBorder		border_;	//!< can be set
    uiRect		workrect_;	//!< will be filled
    bool		zinft_;		//!< From SI()
    DBKey		descid_;

    uiGraphicsScene&	scene();
    void		setDescStartDepth();
    void		putTopInfo();

    void		reDrawCB(CallBacker*);
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
	virtual const LayerSequenceGenDesc* editedDesc() const
										{ return &editdesc_; }

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

