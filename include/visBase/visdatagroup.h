#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "sets.h"
#include "visdata.h"

class SoGroup;
class SoSeparator;

namespace osg { class Group; }

namespace visBase
{

mExpClass(visBase) DataObjectGroup : public DataObject
{
public:

    static DataObjectGroup*	create()
				mCreateDataObj(DataObjectGroup);

    void			setSeparate( bool yn=true ) { separate_=yn; }
				/*! Set directly after creation, othersize
				    it won't have effect */

    virtual int			size() const;
    virtual void		addObject(DataObject*);
    void			addObject(VisID);
				/*!< Calls addObject( DataObject* ) */

    virtual void		insertObject(int idx,DataObject*);

    virtual int			getFirstIdx(VisID) const;
				/*!<\returns	the first index (there might be
						many instances) of the id in
						the group, or -1 if not found
				*/
    virtual int			getFirstIdx(const DataObject*) const;
				/*!<\returns	the first index (there might be
						many instances) of the object in
						the group, or -1 if not found
				*/

    virtual void		removeObject(int idx);
    virtual void		removeAll();
    virtual DataObject*		getObject( int idx )
				{return objects_.size()>idx ? objects_[idx] :0;}
    const DataObject*		getObject( int idx ) const
				{ return const_cast<DataObjectGroup*>(this)->
				    getObject( idx ); }

    void			setDisplayTransformation(const mVisTrans*)
								 override;
    const mVisTrans*		getDisplayTransformation() const override;
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

    virtual			~DataObjectGroup();

    osg::Group*			osggroup_;

    bool			righthandsystem_ = true;
    float			pixeldensity_;

private:

    void			handleNewObj(DataObject*);

    RefObjectSet<DataObject>	objects_;
    bool			separate_ = true;

};

} // namespace visBase
