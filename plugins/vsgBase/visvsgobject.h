#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdata.h"

#include "position.h"

namespace vsg { class Group; }

namespace dGBVis
{

/*!\brief Base class for all objects that are visual on the scene. */

mExpClass(vsgBase) VisualObject : public DataObject
{
public:

    virtual bool		getBoundingBox(Coord3& min,Coord3& max) const;
				/*!<In the coordinates before the display
				    transformation. */

    void			setSelectable( bool yn ) { isselectable_=yn; }
    bool			selectable() const override
				{ return isselectable_; }

    NotifierAccess*		selection() override   { return &selnotifier_; }
    NotifierAccess*		deSelection() override
				{ return &deselnotifier_; }
    NotifierAccess*		rightClicked() override { return &rightclick_; }
    void			rightClick()		{ triggerRightClick(); }

protected:
				VisualObject(bool selectable=false);
				~VisualObject();

    void			triggerSel() override
				{ if (isselectable_) selnotifier_.trigger(); }
    void			triggerDeSel() override
				{ if (isselectable_) deselnotifier_.trigger(); }
    void			triggerRightClick() override
				{ rightclick_.trigger(); }

private:
    bool			isselectable_;
    Notifier<VisualObject>	selnotifier_;
    Notifier<VisualObject>	deselnotifier_;
    Notifier<VisualObject>	rightclick_;
};


mExpClass(vsgBase) VisualObjectImpl : public VisualObject
{
public:

    void		setRightHandSystem( bool yn ) override
			{ righthandsystem_=yn; }
    bool		isRightHandSystem() const override
			{ return righthandsystem_; }

    static const char*	sKeyIsOn();

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:
			VisualObjectImpl(bool selectable);
			~VisualObjectImpl();

    int			addChild(vsg::Node*);
			//!<\returns new child index
    void		insertChild(int pos,vsg::Node*);
    void		removeChild(vsg::Node*);
    int			childIndex(const vsg::Node*) const;

    virtual void	setGroupNode(vsg::Group*);
			//!<Must be called during construction.

    bool		righthandsystem_ = true;

private:

    vsg::Group*		vsgroot_;
};

} // namespace dGBVis
