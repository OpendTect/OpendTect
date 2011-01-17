#ifndef uistratlayseqattrsetbuild_h
#define uistratlayseqattrsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratlayseqattrsetbuild.h,v 1.2 2011-01-17 15:59:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "objectset.h"
class PropertyRef;
class uiListBox;
class uiToolButton;
class CtxtIOObj;
namespace Strat
{
    class RefTree;
    class LayerModel;
    class LaySeqAttrib;
    class LaySeqAttribSet;
}


/*!\brief allows user to define (or read) a set of layer sequence attributes */

mClass uiStratLaySeqAttribSetBuild : public uiGroup
{
public:
    			uiStratLaySeqAttribSetBuild(uiParent*,
						const Strat::LayerModel&);
    			~uiStratLaySeqAttribSetBuild();

    const Strat::LaySeqAttribSet& attribSet() const	{ return attrset_; }
    bool		haveUserChange()		{ return usrchg_; }

protected:

    Strat::LaySeqAttribSet&	attrset_;
    const Strat::RefTree&	reftree_;
    bool			usrchg_;
    ObjectSet<const PropertyRef> props_;
    CtxtIOObj&			ctio_;

    uiListBox*		propfld_;
    uiListBox*		attrfld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	savebut_;

    void		fillPropFld(const Strat::LayerModel&);
    void		fillAttrFld();
    bool		doAttrEd(Strat::LaySeqAttrib&,bool);
    bool		doSetIO(bool);
    void		updButStates();

    void		attrSelChg(CallBacker*);
    void		addReq(CallBacker*);
    void		edReq(CallBacker*);
    void		rmReq(CallBacker*);
    void		openReq(CallBacker*);
    void		saveReq(CallBacker*);

};


#endif
