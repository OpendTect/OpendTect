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

mExpClass(uiTools) uiZAxisTransform : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransform);
public:
    mDefineFactory3ParamInClass(uiZAxisTransform,uiParent*,
				const char*,const char*,factory);
				~uiZAxisTransform();

    virtual void		enableTargetSampling();
    virtual bool		getTargetSampling(StepInterval<float>&) const;

    virtual ZAxisTransform*	getSelection()			= 0;
    virtual StringView 	toDomain() const		= 0;
    virtual StringView 	fromDomain() const		= 0;
    virtual bool		canBeField() const		= 0;
				/*!Returns true if it can be in one line,
				   i.e. as a part of a field. If true,
				   object should look at
				   uiZAxisTransformSel::isField() at
				   construction.
				 */

    void			setIs2D( bool yn ) { is2dzat_ = yn; }
    bool			is2D() const { return is2dzat_; }

protected:
    static bool 		isField(const uiParent*);
				uiZAxisTransform(uiParent*);
    void			rangeChangedCB(CallBacker*);
    void			finalizeDoneCB(CallBacker*);

    uiGenInput* 		rangefld_;
    bool			rangechanged_;
    bool			is2dzat_ = false;
};


/*!Selects a ZAxisTransform. */
mExpClass(uiTools) uiZAxisTransformSel : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransformSel);
public:
				uiZAxisTransformSel(uiParent*, bool withnone,
						    const char* fromdomain=0,
						    const char* todomain=0,
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
    StringView 		selectedToDomain() const;
				/*<!Always available. */

    bool			acceptOK() override;
				/*!<Checks that all input is OK. After that
				    the getSelection will return something. */
    ZAxisTransform*		getSelection();
				/*!<Only after successful acceptOK() */

    bool			getTargetSampling(StepInterval<float>&) const;
				/*!<Only after successful acceptOK and only if
				    withsampling was specified in constructor*/

    bool			fillPar(IOPar&);
				/*!<Only after successful acceptOK() */

protected:
    void			selCB(CallBacker*);

    bool			isfield_;
    BufferString		fromdomain_;
    uiGenInput*			selfld_;
    ObjectSet<uiZAxisTransform>	transflds_;
};
