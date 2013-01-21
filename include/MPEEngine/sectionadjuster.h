#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "task.h"
#include "cubesampling.h"
#include "emposid.h"


class IOPar;
namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }

namespace MPE
{

class SectionExtender;

/*!
\brief SequentialTask to adjust the section of an EM object with ID
EM::SectionID.
*/

mExpClass(MPEEngine) SectionAdjuster : public SequentialTask
{
public:
				SectionAdjuster( const EM::SectionID& sid=-1);
    EM::SectionID		sectionID() const;

    virtual void		reset() {};

    void			setPositions(const TypeSet<EM::SubID>& targets,
	   				     const TypeSet<EM::SubID>* src=0 );
    void			setReferencePosition( const EM::SubID* pos )
    				    { refpos_ = pos; }

    int				nextStep();
    const char*			errMsg() const;

    virtual CubeSampling	getAttribCube(const Attrib::SelSpec&) const;
    				/*!<\returns the cube in which I need the
				     given attrib to track in activevolum. */
    virtual void		getNeededAttribs(
	    			    ObjectSet<const Attrib::SelSpec>&) const;
    virtual bool		is2D() const			{ return false;}
    virtual bool		hasInitializedSetup() const	{ return true; }

    virtual int			getNrAttributes() const		{ return 0; }
    virtual const Attrib::SelSpec* getAttributeSel( int idx ) const { return 0;}
    virtual void		setAttributeSel( int idx,
	    					 const Attrib::SelSpec& ) {}

    void			setThresholdValue(float val);
    float			getThresholdValue() const;
    bool			removeOnFailure(bool yn);
    				/*!<If true, tracked nodes that does not
				    meet certain constraits, e.g. thresholds,
				    are removed. If not, the initial value
				    is kept.
				    \returns the previous status. */
    bool			removesOnFailure() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void			setUndo(bool yn) { setundo_ = yn; }

protected:
    TypeSet<EM::SubID>		pids_;
    TypeSet<EM::SubID>		pidsrc_;
    BufferString		errmsg_;
    EM::SectionID		sectionid_;
    float			thresholdval_;
    bool			removeonfailure_;

    const EM::SubID*		refpos_;
    bool			setundo_;
    
    static const char*		sKeyAdjuster();
    static const char*		sKeyThreshold();
    static const char*		sKeyRemoveOnFailure();
};

}; // namespace MPE

#endif

