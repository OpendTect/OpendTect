#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "factory.h"
#include "stringview.h"
#include "uidlggroup.h"

class ZAxisTransform;
class uiGenInput;
class uiZAxisTransformSel;

/*! Base class for ZAxisTransform ui's*/

mExpClass(uiTools) uiZAxisTranformSetup
{
public:
    StringView		    fromdomain_;
    StringView		    todomain_;
    OD::Pol2D3D		    datatype_	    = OD::Both2DAnd3D;
};

mExpClass(uiTools) uiZAxisTransform : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransform);
public:
    mDefineFactory2ParamInClass(uiZAxisTransform,uiParent*,
				const uiZAxisTranformSetup&,factory);
				~uiZAxisTransform();

    virtual void		enableTargetSampling();
    virtual bool		getTargetSampling(ZSampling&) const;

    virtual StringView		toDomain() const		= 0;
    virtual StringView		fromDomain() const		= 0;

    virtual ZAxisTransform*	getSelection()			= 0;
    virtual bool		canBeField() const		= 0;
				/*!Returns true if it can be in one line,
				   i.e. as a part of a field. If true,
				   object should look at
				   uiZAxisTransformSel::isField() at
				   construction.
				 */

protected:
				uiZAxisTransform(uiParent*);

    static bool			isField(const uiParent*);
};


/*!Selects a ZAxisTransform. */
mExpClass(uiTools) uiZAxisTransformSel : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransformSel);
public:
				uiZAxisTransformSel(uiParent*,bool withnone,
					    const char* fromdomain=nullptr,
					    const char* todomain=nullptr,
					    bool withsampling=false,
					    bool asfield=false,
					    bool is2d=false);
				~uiZAxisTransformSel();

    bool			isField() const;
				/*!<If true, the shape will be a one-line
				    group, with label, combobox and transform
				    settings */
    void			setLabel(const uiString&);

    bool			isOK() const { return nrTransforms(); }
    int				nrTransforms() const;

    NotifierAccess*		selectionDone();
    StringView			selectedToDomain() const;
				/*<!Always available. */

    bool			acceptOK() override;
				/*!<Checks that all input is OK. After that
				    the getSelection will return something. */
    ZAxisTransform*		getSelection();
				/*!<Only after successful acceptOK() */

    bool			getTargetSampling(ZSampling&) const;
				/*!<Only after successful acceptOK and only if
				    withsampling was specified in constructor*/

    bool			fillPar(IOPar&);
				/*!<Only after successful acceptOK() */

protected:
    void			initGrp(CallBacker*);
    void			selCB(CallBacker*);

    bool			isfield_;
    BufferString		fromdomain_;
    uiGenInput*			selfld_ = nullptr;
    ObjectSet<uiZAxisTransform>	transflds_;
};
