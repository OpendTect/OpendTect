#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"

#include "uiioobjsel.h"

class uiPushButton;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiPSProcObjSel : public uiIOObjSel
{
public:
			uiPSProcObjSel(uiParent*,const uiString& seltxt,
				       OD::GeomSystem);
			uiPSProcObjSel(uiParent*,const uiIOObjSel::Setup&,
				       OD::GeomSystem);
			~uiPSProcObjSel();

    static StringView	reqType(OD::GeomSystem);
    static const IOObjContext& ioContext(OD::GeomSystem);

private:

    const OD::GeomSystem gs_;

};


mExpClass(uiPreStackProcessing) uiProcSel : public uiGroup
{ mODTextTranslationClass(uiProcSel);
public:
			uiProcSel(uiParent*,const uiString& label,
				  OD::GeomSystem,int openidx=-1,
				  const uiStringSet* usemethods=nullptr);
			/*/< param openidx on popup, launch the GUI of
				   this step index
			     param usemethods selection of items from
				PreStack::Processor::factory()
				(using all if not provided)	  */

    void		setSel(const MultiID&);
    bool		getSel(MultiID&,bool noerr=false) const;

    Notifier<uiProcSel> selectionDone;

protected:
			~uiProcSel();

    void		initGrpCB(CallBacker*);
    void		editPushCB(CallBacker*);
    void		selDoneCB(CallBacker*);
    bool		checkInput(const IOObj&) const;

    const OD::GeomSystem gs_;
    int			openidx_;
    uiStringSet		usemethods_;
    uiPSProcObjSel*	selfld_;
    uiPushButton*	editbut_;
};


} // namespace PreStack
