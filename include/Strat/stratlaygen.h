#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2011
________________________________________________________________________


-*/

#include "stratmod.h"
#include "property.h"


namespace Strat
{
class RefTree;
class LayerSequence;
class LayerSequenceGenDesc;

/*!\brief Description that can generate layers and add these to a sequence.
 
  To be able to display this to users, youneed to be able to return a
  non-varying thickness. This dispThickness() can be the maximum possible
  (or if that is impossible, like 2 std devs), or an average/typical/center
  value.
 
 */

mExpClass(Strat) LayerGenerator
{
public:	

    virtual		~LayerGenerator()			{}

    virtual LayerGenerator* clone() const
			{ return canBeCloned() ? createClone() : 0; }
    virtual bool	canBeCloned() const			= 0;
    virtual const char* name() const				= 0;
    virtual float	dispThickness(bool max=false) const	= 0;

    virtual bool	usePar(const IOPar&,const RefTree&);
    virtual void	fillPar(IOPar&) const;

    static LayerGenerator* get(const IOPar&,const RefTree&);
    mDefineFactoryInClass(LayerGenerator,factory);

    bool		generateMaterial(LayerSequence&,
			    Property::EvalOpts eo=Property::EvalOpts()) const;

    virtual bool	reset() const	{ return true; }
    virtual uiString	errMsg() const	{ return uiString::emptyString(); }
    virtual void	syncProps(const PropertyRefSelection&)		= 0;
    virtual void	updateUsedProps(PropertyRefSelection&) const	= 0;

    void		setGenDesc( LayerSequenceGenDesc* gd )	{ gendesc_=gd; }

protected:

    virtual LayerGenerator*	createClone() const		{ return 0; }
    virtual bool	genMaterial(LayerSequence&,Property::EvalOpts) const
							= 0;
    virtual bool	postProcess(LayerSequence&,float pos) const
							{ return true; }

    const LayerSequenceGenDesc* gendesc_; //!< set before generation
    friend class	LayerSequenceGenDesc;

};


#define mDefLayerGeneratorFns(clss,typstr) \
protected: \
    bool		genMaterial(Strat::LayerSequence&, \
			    Property::EvalOpts eo=Property::EvalOpts() \
			    ) const override; \
public: \
    static const char*	typeStr()		{ return typstr; } \
    const char*		factoryKeyword() const override { return typeStr(); } \
    static Strat::LayerGenerator* create()	{ return new clss; } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    const char*		name() const override; \
    float		dispThickness(bool max=true) const override; \
    bool		usePar(const IOPar&,const Strat::RefTree&) override; \
    void		fillPar(IOPar&) const override; \
    void		syncProps(const PropertyRefSelection&) override; \
    void		updateUsedProps(PropertyRefSelection&) const override



}; // namespace Strat

