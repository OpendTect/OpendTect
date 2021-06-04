#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
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
    virtual void		addObject( DataObject* );
    void			addObject( int id );
    				/*!< Calls addObject( DataObject* ) */

    virtual void		insertObject( int idx, DataObject* );

    virtual int			getFirstIdx( int id ) const;
    				/*!<\returns	the first index (there might be
						many instances) of the id in
						the group, or -1 if not found 
				*/
    virtual int			getFirstIdx( const DataObject* ) const;
    				/*!<\returns	the first index (there might be
						many instances) of the object in
						the group, or -1 if not found 
				*/

    virtual void		removeObject( int idx );
    virtual void		removeAll();
    virtual DataObject*		getObject( int idx )
    				{return objects_.size()>idx ? objects_[idx] :0;}
    const DataObject*		getObject( int idx ) const
				{ return const_cast<DataObjectGroup*>(this)->
				    getObject( idx ); }

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    				/*!\returns the trans of the first child
					    with a trans, or null if none of
					    the childrens has a trans */
    void			setRightHandSystem(bool);
    bool			isRightHandSystem() const;

    void			setPixelDensity(float dpi);
    float			getPixelDensity() const { return pixeldensity_;}

    Notifier<DataObjectGroup>	change; //triggers on add/insert/remove
protected:

    virtual			~DataObjectGroup();

    osg::Group*			osggroup_;

    bool			righthandsystem_;
    float			pixeldensity_;

protected:

    ObjectSet<DataObject>	objects_;
    bool			separate_;

};

} //namespace

