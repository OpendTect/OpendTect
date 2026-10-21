#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdata.h"

#include "refcount.h"

namespace vsg { class Group; }

namespace dGBVis
{

mExpClass(vsgBase) DataObjectGroup : public DataObject
{
public:

    static RefMan<DataObjectGroup> create();
				mCreateDGBVisObj(DataObjectGroup);

    virtual int			size() const;
    virtual void		addObject(DataObject*);
    void			addObject(const VisID&);
				/*!< Calls addObject( DataObject* ) */
    virtual void		insertObject(int idx,DataObject*);

    virtual int			getFirstIdx(const VisID&) const;
    virtual int			getFirstIdx(const DataObject*) const;

    virtual void		removeObject(int idx);
    virtual void		removeAll();
    virtual DataObject*		getObject(int idx);
    const DataObject*		getObject(int idx) const;

    void			setDisplayTransformation(const mDGBVisTrans*)
								override;
    const mDGBVisTrans*		getDisplayTransformation() const override;
				/*!\returns the trans of the first child
					    with a trans, or null if none of
					    the childrens has a trans */
    void			setRightHandSystem(bool) override;
    bool			isRightHandSystem() const override;

    void			setPixelDensity(float dpi) override;
    float			getPixelDensity() const override
				{ return pixeldensity_; }

    Notifier<DataObjectGroup>	change; //triggers on add/insert/remove

protected:
				DataObjectGroup(vsg::Group*);
				//!<For subclasses with their own group node
				~DataObjectGroup();

    vsg::Group*			vsggroup_;

    bool			righthandsystem_ = true;
    float			pixeldensity_;

private:

    void			handleNewObj(DataObject*);

    RefObjectSet<DataObject>	objects_;
};

} // namespace dGBVis
