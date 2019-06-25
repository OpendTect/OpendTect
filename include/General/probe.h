#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"
#include "integerid.h"
#include "odpresentationmgr.h"
#include "saveable.h"
#include "sharedobject.h"
#include "trckeyzsampling.h"

class IOPar;
class IOObj;
class Probe;
class ProbeManager;

mExpClass(General) ProbeLayer : public SharedObject
{
public:

    mDefIntegerIDType(	ID);

			ProbeLayer();
			mDeclAbstractMonitorableAssignment(ProbeLayer);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    virtual void	invalidateData()		{}
    virtual const char* layerType() const		= 0;

    virtual bool	haveSavedDispPars() const	{ return false; }
    virtual void	saveDisplayPars()		{}

    ID			getID() const;
    const Probe*	getProbe() const;
    static ID		getNewID();

    static const char*	sKeyLayerType();
    static const char*	sKeyLayer();

protected:

			~ProbeLayer()			{}

    const ID		id_;
    ConstRefMan<Probe>	probe_;

private:

    void		setProbe(const Probe*);

    friend class	Probe;

};


mExpClass(General) ProbeLayerFactory
{
public:

    typedef ProbeLayer* (*CreateFunc)(const IOPar&);

    void		addCreateFunc(CreateFunc,const char*);
    ProbeLayer*		create(const IOPar&);

protected:

    TypeSet<CreateFunc>			createfuncs_;
    BufferStringSet			keys_;

};

mGlobal(General) ProbeLayerFactory& PrLayFac();


mExpClass(General) Probe : public SharedObject
{
public:

			Probe();
			mDeclAbstractMonitorableAssignment( Probe );
			mDeclInstanceCreatedNotifierAccess( Probe );

    static const char*	sProbeType();
    static ChangeType	cPositionChange()		{ return 2; }
    static ChangeType	cDimensionChange()		{ return 3; }
    static ChangeType	cLayerAdd()			{ return 4; }
    static ChangeType	cLayerToBeRemoved()		{ return 5; }
    static ChangeType	cLayerChange()			{ return 6; }

    void		setPos(const TrcKeyZSampling&);
    virtual const char*	type() const			= 0;
    virtual uiWord	usrType() const			= 0;
    virtual uiWord	displayName() const		= 0;

    virtual bool	is2D() const			{ return false; }
    virtual bool	isVertical() const		{ return true; }
    virtual bool	is3DSlice() const		{ return false; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    mImplSimpleMonitoredGet(position,TrcKeyZSampling,probepos_);

    int			nrLayers() const;
    void		addLayer(ProbeLayer*);
    ProbeLayer*		removeLayer(ProbeLayer::ID);
    const ProbeLayer*	getLayerByIdx(int) const;
    ProbeLayer*		getLayerByIdx(int);
    const ProbeLayer*	getLayer(ProbeLayer::ID) const;
    ProbeLayer*		getLayer(ProbeLayer::ID);

protected:

				~Probe();

    ObjectSet<ProbeLayer>	layers_;
    TrcKeyZSampling		probepos_;

    uiWord			mkDispNm(const uiWord& add) const;
				//!< generates join of type and '[add]'
    template<class T>
    inline uiWord		mkDispNm( const T& t ) const
				{ return mkDispNm( toUiString(t) ); }

};


#define mDeclRequiredProbeFns() \
    static const char*	sFactoryKey(); \
    static Probe*	createFrom(const IOPar&); \
    static void		initClass(); \
 \
    const char*	type() const override		{ return sFactoryKey(); } \
    uiWord	usrType() const override; \
    uiWord	displayName() const override



mExpClass(General) ProbeFactory
{
public:

    typedef Probe*	(*CreateFunc)(const IOPar&);

    void		addCreateFunc(CreateFunc,const char*);
    Probe*		create(const IOPar&);

protected:

    TypeSet<CreateFunc>			createfuncs_;
    BufferStringSet			keys_;

};


mGlobal(General) ProbeFactory& ProbeFac();


mExpClass(General) ProbeSaver : public Saveable
{
public:
			ProbeSaver(const Probe&);
			mDeclMonitorableAssignment(ProbeSaver);
			~ProbeSaver();
protected:

    virtual uiRetVal	doStore(const IOObj&,
				const TaskRunnerProvider&) const override;

};


mExpClass(General) ProbePresentationInfo : public Presentation::ObjInfo
{
public:
					ProbePresentationInfo(const DBKey&);
					ProbePresentationInfo();

    static Presentation::ObjInfo*	createFrom(const IOPar&);
    static void				initClass();
    static const char*			sFactoryKey();

};
