/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "horizoneditor.h"
#include "geeditorimpl.h"
#include "binidsurface.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "undo.h"
#include "mpeengine.h"

#define mSinus		0
#define mPyramid	1
#define mBox		2

namespace MPE
{

HorizonEditor::HorizonEditor( EM::Horizon3D& horizon )
    : ObjectEditor(horizon)
    , editarea( 5, 5 )
    , horbox( false )
    , vertstyle( 0 )
{
    emobject.change.notify( mCB(this,HorizonEditor,emChangeCB) );
    vertstylenames.add("Sinus").add("Pyramid").add("Box");
}


HorizonEditor::~HorizonEditor()
{
    emobject.change.remove( mCB(this,HorizonEditor,emChangeCB) );
}


ObjectEditor* HorizonEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Horizon3D*,horizon,&emobj);
    return horizon ? new HorizonEditor( *horizon ) : 0;
}


void HorizonEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Horizon3D::typeStr() ); }


Geometry::ElementEditor* HorizonEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::BinIDSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::BinIDElementEditor( 
		*const_cast<Geometry::BinIDSurface*>(surface) );
}


void HorizonEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    // TODO: display seeds and duplicate every 10th inline or so.
    ids.erase();

    const TypeSet<EM::PosID>* pids =
	emobject.getPosAttribList( EM::EMObject::sPermanentControlNode() );

    if ( pids ) ids = *pids;

    pids = emobject.getPosAttribList( EM::EMObject::sTemporaryControlNode() );
    if ( pids ) ids.createUnion( *pids );
}


bool HorizonEditor::addEditID( const EM::PosID& pid )
{
    emobject.setPosAttrib( pid, EM::EMObject::sPermanentControlNode(), true,true);
    EM::EMM().undo().setUserInteractionEnd(
	    EM::EMM().undo().currentEventID() );
    return true;
}


bool HorizonEditor::removeEditID( const EM::PosID& pid )
{
    emobject.setPosAttrib( pid, EM::EMObject::sPermanentControlNode(),false,true);
    return true;
}


const BufferStringSet* HorizonEditor::getAlongMovingStyleNames() const
{ return &vertstylenames;}


void HorizonEditor::getAlongMovingNodes( const EM::PosID&,
					 TypeSet<EM::PosID>& nodes,
					 TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors ) factors->erase();

    if ( movingnode.objectID()==-1 )
	return;

    mDynamicCastGet(const EM::Horizon3D*,horizon,&emObject())
    if ( !horizon )
	return;

    const EM::SectionID sectionid = movingnode.sectionID();
    const RowCol rc = movingnode.getRowCol();
    const RowCol step = horizon->geometry().step();

    for ( int ridx=-editarea.row; ridx<=editarea.row; ridx++ )
    {
	for ( int cidx=-editarea.col; cidx<=editarea.col; cidx++ )
	{
	    float effect = 1;
	    if ( !horbox )
	    {
		const float relrow = (float)ridx/editarea.row;
		const float relcol = (float)cidx/editarea.col;
		const float curradius = Math::Sqrt(relrow*relrow+relcol*relcol);
		if ( curradius>1 ) 
		    continue;

		effect = 1.0f - curradius;
	    }
	    else
	    {
		const float length = Math::Sqrt( (float)(ridx*ridx+cidx*cidx) );

		if ( ridx )
		{
		    const float rowfactor = fabs((float)ridx)/(editarea.row+1);
		    const float colfactor = fabs((float)cidx)/rowfactor;
		    const float totallength =
			Math::Sqrt(editarea.row*editarea.row+colfactor*colfactor);

		    effect = 1-length/totallength;
		}

		if ( cidx )
		{
		    const float colfactor = fabs((float)cidx)/(editarea.col+1);
		    const float rowfactor = fabs((float)ridx)/colfactor;
		    const float totallength =
			Math::Sqrt(editarea.col*editarea.col+rowfactor*rowfactor);

		    const float neweffect = 1-length/totallength;
		    effect = mMIN(neweffect,effect);
		}
	    }

	    if ( vertstyle==mSinus )
		effect = (float) sin( effect*M_PI_2 );
	    else if ( vertstyle==mBox )
		effect = mCast( float, effect ? 1 : 0 );
	    
	    const RowCol currc =  rc + step*RowCol(ridx,cidx);
	    if ( horizon->isDefined(sectionid,currc.toInt64()) )
	    {
		nodes += EM::PosID( horizon->id(), sectionid,
					     currc.toInt64() );
		if ( factors ) (*factors) += effect;
	    }
	}
    }
}


void HorizonEditor::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if (  cbdata.event==EM::EMObjectCallbackData::AttribChange )
    {
	if ( cbdata.attrib==EM::EMObject::sPermanentControlNode() ||
	     cbdata.attrib==EM::EMObject::sTemporaryControlNode() )
	{
	    editpositionchange.trigger();
	}
    }
}


Horizon2DEditor::Horizon2DEditor( EM::Horizon2D& hor2d )
    : ObjectEditor(hor2d)
{}

ObjectEditor* Horizon2DEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,&emobj);
    return hor2d ? new Horizon2DEditor( *hor2d ) : 0;
}


void Horizon2DEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Horizon2D::typeStr() ); }


Geometry::ElementEditor* Horizon2DEditor::createEditor(const EM::SectionID& sid)
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::BinIDSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::BinIDElementEditor( 
		*const_cast<Geometry::BinIDSurface*>(surface) );
}




}; // namespace MPE
