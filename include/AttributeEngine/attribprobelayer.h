#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "attribsel.h"
#include "probe.h"
#include "datapack.h"
#include "coltabmapper.h"
#include "coltabsequence.h"

class SeisIOObjInfo;

mExpClass(AttributeEngine) AttribProbeLayer : public ProbeLayer
{
public:

    typedef ColTab::Sequence	Sequence;
    typedef ColTab::Mapper	Mapper;

    enum DispType		{ VD, Wiggle, RGB };

				AttribProbeLayer(DispType dt=VD);
				~AttribProbeLayer();
				mDeclMonitorableAssignment(AttribProbeLayer);

    const char*			layerType() const;
    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    bool			hasData() const
				{ return attribdpid_.isValid(); }
    virtual void		invalidateData();
    virtual DataPack::MgrID	getDataPackManagerID() const
				{ return DataPackMgr::SeisID(); }

    mImplSimpleMonitoredGet(	selSpec,Attrib::SelSpec,attrspec_)
    mImplSimpleMonitoredGet(	dataPackID,DataPack::ID,attribdpid_)
    mImplSimpleMonitoredGet(	dispType,DispType,disptype_)
    mImplSimpleMonitoredGet(	selectedComponent,int,selcomp_)

    const Sequence&		sequence() const;
    void			setSequence(const Sequence&);
    Mapper&			mapper()		{ return *mapper_; }
    const Mapper&		mapper() const		{ return *mapper_; }
    void			setSelSpec(const Attrib::SelSpec&);
    int				nrAvialableComponents() const;
    void			setSelectedComponent(int);
    void			setDataPackID(DataPack::ID);

    bool			haveSavedDispPars() const;
    void			saveDisplayPars();

    static ProbeLayer*		createFrom(const IOPar&);
    static void			initClass();

    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cSelSpecChg()		{ return 3; }
    static ChangeType		cColSeqChange()		{ return 4; }
    static ChangeType		cMapperChange()		{ return 5; }
    static ChangeType		cSelCompChange()	{ return 6; }

    static const char*		sFactoryKey();

protected:

    Attrib::SelSpec&		attrspec_;
    DispType			disptype_;
    DataPack::ID		attribdpid_;
    ConstRefMan<DataPack>	attrdp_;
    int				selcomp_;
    ConstRefMan<Sequence>	colseq_;    //!< null by default
    RefMan<Mapper>		mapper_;    //!< never null

    void			usePar4ColTab(const IOPar&);
    void			handleDataPackChange();
    void			handleSelSpecChg();
    SeisIOObjInfo*		gtSeisInfo() const;

};
