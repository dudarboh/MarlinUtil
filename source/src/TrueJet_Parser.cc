#include "TrueJet_Parser.h"
#include <stdlib.h>
#include <math.h>
//#include <cmath>
#include <iostream>
#include <iomanip>


// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#ifdef MARLIN_USE_AIDA
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
//#include <AIDA/IHistogram1D.h>
#endif // MARLIN_USE_AIDA


using namespace lcio ;
using namespace marlin ;

struct MCPseen : LCIntExtension<MCPseen> {} ;


  TrueJet_Parser::TrueJet_Parser() {

    m_intvec=new IntVec()    ;
    m_mcpartvec=new MCParticleVec()    ;
    _COUNT_FSR=1;
}

TrueJet_Parser::~TrueJet_Parser() {
}



//*************************///
const double* TrueJet_Parser::p4seen(int ijet) {
  m_p4[0]=Eseen(ijet);
  const double* mom = pseen(ijet);
   for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
   }
   return m_p4 ;
}


double TrueJet_Parser::Etrue(int ijet) {
  static FloatVec www ;
  LCObjectVec mcpvec = reltjmcp->getRelatedToObjects( jets->at(ijet) );
  www = reltjmcp->getRelatedToWeights( jets->at(ijet));
  double E=0.0;
  for ( unsigned kk=0 ; kk<mcpvec.size() ; kk++ ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(mcpvec[kk]);
    if ( _COUNT_FSR ) {
      if (  abs(www[kk]) == 1.0) {
        E+=mcp->getEnergy();
      }
    } else {
      if (  www[kk] == 1.0) {
        E+=mcp->getEnergy();
      }
    }
  }
  return E;
}
double TrueJet_Parser::Mtrue(int ijet) {
  const double* p_4 = p4true(ijet);
  double psqr=0;
  for (int kk=1 ; kk<=3 ; kk++ ) {
     psqr+=p_4[kk]*p_4[kk];
  }
  double M=sqrt(p_4[0]*p_4[0]-psqr);
  return M ;
}
const double* TrueJet_Parser::ptrue(int ijet) {
  static FloatVec www ;
  LCObjectVec mcpvec = reltjmcp->getRelatedToObjects( jets->at(ijet) );
  www =  reltjmcp->getRelatedToWeights( jets->at(ijet));
  m_p3[0]=0.; m_p3[1]=0.; m_p3[2]=0.;
  for ( unsigned kk=0 ; kk<mcpvec.size() ; kk++ ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(mcpvec[kk]);
    if ( _COUNT_FSR ) {
      if (  abs(www[kk]) == 1.0) {
        const double* mom = mcp->getMomentum();
        for (int jj=0 ; jj<3 ; jj++ ) {
          m_p3[jj]+=mom[jj];
        }
      }
    } else {
      if (  www[kk] == 1.0) {
        const double* mom = mcp->getMomentum();
        for (int jj=0 ; jj<3 ; jj++ ) {
          m_p3[jj]+=mom[jj];
        }
      }
    }
  }
  return m_p3;
}

const double* TrueJet_Parser::p4true(int ijet) {
  const double* mom = ptrue(ijet);
  for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
  }
  m_p4[0]=Etrue(ijet);
  return m_p4 ;
}

const MCParticleVec& TrueJet_Parser::true_partics(int ijet) {
  m_mcpartvec->clear();
  MCParticleVec* jetmcps=m_mcpartvec;
  LCObjectVec jetmcpsx = reltjmcp->getRelatedToObjects( jets->at(ijet) );
  for ( unsigned iii=0 ; iii < jetmcpsx.size() ; iii++ ) {
    jetmcps->push_back(dynamic_cast<MCParticle*>(jetmcpsx[iii]));
  }
  return *jetmcps;
}

const double* TrueJet_Parser::pquark(int ijet) {
  if (final_elementon(ijet) != NULL ) {
    return final_elementon(ijet)->getMomentum() ;
  } else {
    m_p3[0]=0. ;m_p3[1]=0. ;m_p3[2]=0. ;
    return m_p3 ;
  }
}

const double* TrueJet_Parser::p4quark(int ijet) {
  const double* mom = pquark(ijet);
  for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
  }
  m_p4[0]=Equark(ijet);
  return m_p4 ;
}


double TrueJet_Parser::Etrueseen(int ijet) {
  if (  reltrue_tj == 0 ) {
    LCCollection* rmclcol = NULL;
    try{
     rmclcol = m_evt->getCollection( get_recoMCTruthLink() );
    }
    catch( lcio::DataNotAvailableException& e )
    {
      streamlog_out(WARNING) << get_recoMCTruthLink()   << " collection not available" << std::endl;
        rmclcol = NULL;
    }
    reltrue_tj = new LCRelationNavigator( rmclcol );
  }
  LCObjectVec mcpvec = reltjmcp->getRelatedToObjects( jets->at(ijet) );
  double E=0.0;
  for ( unsigned kk=0 ; kk<mcpvec.size() ; kk++ ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(mcpvec[kk]);
    LCObjectVec recovec = reltrue_tj->getRelatedFromObjects( mcp);
    if ( recovec.size() > 0 ) { // if reconstructed
      if ( mcp->getParents().size() == 0 || ! ( mcp->getParents()[0]->ext<MCPseen>() == 1) ) {  // if ancestor not already counted
        E+=mcp->getEnergy();
      }
      mcp->ext<MCPseen>() = 1 ;
    } else {
      if (  mcp->getParents().size() > 0 && mcp->getParents()[0]->ext<MCPseen>() == 1 ) {   // if parent of particles seen,
                                                                                            // consider the particle itself as seen as seen
        mcp->ext<MCPseen>() = 1 ;
      }
    }
  }
  return E;
}
double TrueJet_Parser::Mtrueseen(int ijet) {
  const double* p_4 = p4trueseen(ijet);
  double psqr=0;
  for (int kk=1 ; kk<=3 ; kk++ ) {
     psqr+=p_4[kk]*p_4[kk];
  }
  double M=sqrt(p_4[0]*p_4[0]-psqr);
  return M ;
}
const double* TrueJet_Parser::ptrueseen(int ijet) {
  if (  reltrue_tj == 0 ) {
    LCCollection* rmclcol = NULL;
     try{
      rmclcol = m_evt->getCollection( get_recoMCTruthLink() );
    }
    catch( lcio::DataNotAvailableException& e )
    {
      streamlog_out(WARNING) << get_recoMCTruthLink()   << " collection not available" << std::endl;
        rmclcol = NULL;
    }
    reltrue_tj = new LCRelationNavigator( rmclcol );
  }
  LCObjectVec mcpvec = reltjmcp->getRelatedToObjects( jets->at(ijet) );
  m_p3[0]=0 ; m_p3[1]=0 ; m_p3[2]=0 ;
  for ( unsigned kk=0 ; kk<mcpvec.size() ; kk++ ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(mcpvec[kk]);
    LCObjectVec recovec = reltrue_tj->getRelatedFromObjects( mcp);
    if ( recovec.size() > 0 ) { // if reconstructed
      if ( mcp->getParents().size() == 0 || ! ( mcp->getParents()[0]->ext<MCPseen>() == 1 ) ) {  // if ancestor not already counted
        const double* mom = mcp->getMomentum();
        for (int jj=0 ; jj<3 ; jj++ ) {
          m_p3[jj]+=mom[jj];
        }
      }
      mcp->ext<MCPseen>() = 1 ;
    } else {
      if (  mcp->getParents().size() > 0 && mcp->getParents()[0]->ext<MCPseen>() == 1 ) {   // if parent of particles seen,
                                                                                            // consider the particle itself as seen as seen
        mcp->ext<MCPseen>() = 1 ;
      }
    }
  }
  return m_p3;
}

const double* TrueJet_Parser::p4trueseen(int ijet) {
  const double* mom = ptrueseen(ijet);
  for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
  }
  m_p4[0]=Etrueseen(ijet);
  return m_p4 ;
}


ReconstructedParticleVec* TrueJet_Parser::getJets(){
  //LCObjectVec* TrueJet_Parser::jets(){



    ReconstructedParticleVec* tjs = new ReconstructedParticleVec();
    //    LCObjectVec* tjs = new LCObjectVec();
    int ntj = tjcol->getNumberOfElements()  ;
    // std::cout << " n jets " << ntj << std::endl;

    for(int j=0; j< ntj ; j++){

      ReconstructedParticle* tj = dynamic_cast<ReconstructedParticle*>( tjcol->getElementAt( j ) ) ;
      tjs->push_back(tj);
      tj->ext<JetIndex>()=j;


    }
    return tjs;
}

 ReconstructedParticleVec* TrueJet_Parser::getFinalcn(){


    ReconstructedParticleVec* fcns = new  ReconstructedParticleVec();
    int nfcn = fcncol->getNumberOfElements()  ;
    //std::cout << " n fcn " << nfcn << std::endl;

    for(int j=0; j< nfcn ; j++){

      ReconstructedParticle* fcn = dynamic_cast<ReconstructedParticle*>( fcncol->getElementAt( j ) ) ;
      fcns->push_back(fcn);
      fcn->ext<FcnIndex>()=j;
    }
    return fcns;
}
 ReconstructedParticleVec* TrueJet_Parser::getInitialcn(){


    ReconstructedParticleVec* icns = new  ReconstructedParticleVec();
    int nicn = icncol->getNumberOfElements()  ;
    //std::cout << " n icn " << nicn << std::endl;

    for(int j=0; j< nicn ; j++){

      ReconstructedParticle* icn = dynamic_cast<ReconstructedParticle*>( icncol->getElementAt( j ) ) ;
      icns->push_back(icn);
      icn->ext<IcnIndex>()=j;
    }
    return icns;
}
const IntVec&  TrueJet_Parser::final_siblings( int ijet ) {
  m_intvec->clear();
  IntVec* sibl=m_intvec;
  LCObjectVec fcnvec = relfcn->getRelatedToObjects( jets->at(ijet) );
  int nsibl=0;
  for ( unsigned kk=0 ; kk<fcnvec.size() ; kk++ ) {
    ReconstructedParticleVec jetvec= dynamic_cast<ReconstructedParticle*>(fcnvec[kk])->getParticles();
    for ( unsigned jj=0 ; jj<jetvec.size() ; jj++ ) {
      if ( jetvec[jj] != jets->at(ijet) ) {
        // sibling
        int jjet=jetvec[jj]->ext<JetIndex>();
        sibl->push_back(jjet);
        nsibl++;
      }
    }
  }
  return *sibl;
  //

}
const IntVec& TrueJet_Parser::initial_siblings( int ijet ){
  m_intvec->clear();
  IntVec* sibl=m_intvec;
  LCObjectVec icnvec = relicn->getRelatedToObjects( jets->at(ijet) );
  int nsibl=0;
  for ( unsigned kk=0 ; kk<icnvec.size() ; kk++ ) {
    ReconstructedParticleVec jetvec= dynamic_cast<ReconstructedParticle*>(icnvec[kk])->getParticles();
    for ( unsigned jj=0 ; jj<jetvec.size() ; jj++ ) {
      if ( jetvec[jj] != jets->at(ijet) ) {
        int jjet=jetvec[jj]->ext<JetIndex>();
        sibl->push_back(jjet);
        nsibl++;
      }
    }
  }
  return *sibl;
  //

}

int TrueJet_Parser::mcpjet( MCParticle* mcp) {
   LCObjectVec jetvec = reltjmcp->getRelatedFromObjects( mcp );
   if (jetvec.size() > 0 ) {
     return  jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0])) ;
   } else {
     return -1000 ;
   }
}

int TrueJet_Parser::mcpicn( MCParticle* mcp) {
   LCObjectVec jetvec = reltjmcp->getRelatedFromObjects( mcp );
   if (jetvec.size() > 0 ) {
     return  final_cn(jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0]))) ;
   } else {
     return -1000 ;
   }

}

int TrueJet_Parser::mcpfcn( MCParticle* mcp) {
   LCObjectVec jetvec = reltjmcp->getRelatedFromObjects( mcp );
   if (jetvec.size() > 0 ) {
     return initial_cn( jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0]))) ;
   } else {
     return -1000 ;
   }

}

int TrueJet_Parser::recojet( ReconstructedParticle* reco) {
   LCObjectVec jetvec = reltjreco->getRelatedFromObjects( reco );
   if (jetvec.size() > 0 ) {
     return  jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0])) ;
   } else {
     return -1000 ;
   }

}

int TrueJet_Parser::recoicn( ReconstructedParticle* reco) {
   LCObjectVec jetvec = reltjreco->getRelatedFromObjects( reco );
   if (jetvec.size() > 0 ) {
     return  final_cn(jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0]))) ;
   } else {
     return -1000 ;
   }

}

int TrueJet_Parser::recofcn( ReconstructedParticle* reco) {
   LCObjectVec jetvec = reltjreco->getRelatedFromObjects( reco );
   if (jetvec.size() > 0 ) {
     return initial_cn( jetindex( dynamic_cast<ReconstructedParticle*>(jetvec[0]))) ;
   } else {
     return -1000 ;
   }

}

int TrueJet_Parser::final_cn( int ijet ) {
   LCObjectVec fcnvec = relfcn->getRelatedToObjects( jets->at(ijet) );
   int fcn ;
   if (fcnvec.size() > 0 ) {
     fcn=fcnvec[0]->ext<FcnIndex>();
   } else {
     fcn = -1 ;
   }
   return fcn;
}
int TrueJet_Parser::initial_cn( int ijet ) {
   LCObjectVec icnvec = relicn->getRelatedToObjects( jets->at(ijet) );
   int icn ;
   if (icnvec.size() > 0 ) {
      icn=icnvec[0]->ext<IcnIndex>();
   } else {
     icn = -1 ;
   }

   return icn;
}

const IntVec&  TrueJet_Parser::jets_of_final_cn( int ifcn ) {
  m_intvec->clear();
  IntVec* jets_of_fcn=m_intvec;
  // way to find: jet-to-icn link icn->reco : jets, find notthis.
  //  index<->jet : LCExtension
  LCObjectVec jetvec = relfcn->getRelatedFromObjects(  finalcns->at(ifcn) );
    // ReconstructedParticleVec jetvec= dynamic_cast<ReconstructedParticle*>(fcnvec[ifcn])->getParticles();
  for ( unsigned kk=0 ; kk<jetvec.size() ; kk++ ) {
    int jjet=jetvec[kk]->ext<JetIndex>();
    jets_of_fcn->push_back(jjet);
  }
  return *jets_of_fcn;
  //

}
const IntVec&  TrueJet_Parser::jets_of_initial_cn( int iicn ) {
  m_intvec->clear();
  IntVec* jets_of_icn=m_intvec;
  // way to find: jet-to-icn link icn->reco : jets, find notthis.
  //  index<->jet : LCExtension
  //LCObjectVec jetvec = relfcn->getRelatedFromObjects(  finalcns->at(ifcn) );
  ReconstructedParticleVec jetvec= dynamic_cast<ReconstructedParticle*>(initialcns->at(iicn))->getParticles();
  for ( unsigned kk=0 ; kk<jetvec.size() ; kk++ ) {
    int jjet=jetvec[kk]->ext<JetIndex>();
    jets_of_icn->push_back(jjet);
  }
  return *jets_of_icn;
  //

}

const IntVec&  TrueJet_Parser::pdg_icn_comps(int iicn) {
  m_intvec->clear();
  for ( unsigned ipid=1 ; ipid <  initialcns->at(iicn)->getParticleIDs().size() ; ipid++ ) {
    m_intvec->push_back( initialcns->at(iicn)->getParticleIDs()[ipid]->getPDG());
  }
  return *m_intvec;
  //

}

const IntVec&  TrueJet_Parser::type_icn_comps(int iicn) {
  m_intvec->clear();
  for ( unsigned ipid=1 ; ipid <  initialcns->at(iicn)->getParticleIDs().size() ; ipid++ ) {
    m_intvec->push_back( initialcns->at(iicn)->getParticleIDs()[ipid]->getType());
  }
  return *m_intvec;
  //

}


const double* TrueJet_Parser::p4_icn(int iicn) {
  const double* mom = p_icn(iicn);
   for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
   }
   m_p4[0]=E_icn(iicn);
   return m_p4 ;
}

const IntVec&  TrueJet_Parser::pdg_fcn_comps(int ifcn) {
  m_intvec->clear();
  for ( unsigned ipid=1 ; ipid <  finalcns->at(ifcn)->getParticleIDs().size() ; ipid++ ) {
    m_intvec->push_back( finalcns->at(ifcn)->getParticleIDs()[ipid]->getPDG());
  }
  return *m_intvec;
  //

}
const IntVec&  TrueJet_Parser::type_fcn_comps(int ifcn) {
  m_intvec->clear();
  for ( unsigned ipid=1 ; ipid <  finalcns->at(ifcn)->getParticleIDs().size() ; ipid++ ) {
    m_intvec->push_back( finalcns->at(ifcn)->getParticleIDs()[ipid]->getType());
  }
  return *m_intvec;
  //

}



const double* TrueJet_Parser::p4_fcn(int ifcn) {
  const double* mom = p_fcn(ifcn);
   for (int kk=1 ; kk<=3 ; kk++ ) {
     m_p4[kk]=mom[kk-1];
   }
   m_p4[0]=E_fcn(ifcn);
   return m_p4 ;
}

MCParticle* TrueJet_Parser::initial_elementon(int ijet){
//  int icn=initial_cn(ijet);
//  LCObjectVec elementonvec = relip->getRelatedToObjects(initialcns->at(icn));
  LCObjectVec elementonvec = relip->getRelatedToObjects(jets->at(ijet));
  if (elementonvec.size() > 0 ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(elementonvec[0]);
    return mcp;
  } else {
    return NULL;
  }

}
MCParticle* TrueJet_Parser::final_elementon(int ijet){
//  int fcn=final_cn(ijet);
//  LCObjectVec elementonvec = relfp->getRelatedToObjects(finalcns->at(fcn));
  LCObjectVec elementonvec = relfp->getRelatedToObjects(jets->at(ijet));
  if (elementonvec.size() > 0 ) {
    MCParticle* mcp  = dynamic_cast<MCParticle*>(elementonvec[0]);
    return mcp;
  } else {
    return NULL;
  }

}
const MCParticleVec& TrueJet_Parser::elementons_final_cn(int ifcn){
  m_mcpartvec->clear();

  MCParticleVec* elementons= m_mcpartvec;
  IntVec jetind=jets_of_final_cn(ifcn );
  for (unsigned kk=0 ; kk <jetind.size() ; kk++ ) {
    MCParticle* mcp=final_elementon(jetind[kk]);
    if ( mcp != NULL ) {
      elementons->push_back(mcp);
    }
  }
  return *elementons;
}

const MCParticleVec& TrueJet_Parser::elementons_initial_cn(int iicn){
  m_mcpartvec->clear();

  MCParticleVec* elementons= m_mcpartvec;
  IntVec jetind=jets_of_initial_cn(iicn );
  for (unsigned kk=0 ; kk <jetind.size() ; kk++ ) {
    MCParticle* mcp=initial_elementon(jetind[kk]);
    if ( mcp != NULL ) {
      elementons->push_back(mcp);
    }
  }
  return *elementons;
}

void TrueJet_Parser::getall( LCEvent * event ) {


    m_evt=event;
     // get TrueJets
    try{
      tjcol = m_evt->getCollection( _trueJetCollectionName);
      if (  tjcol->getNumberOfElements() == 0 ) { return ; }
    }
    catch( lcio::DataNotAvailableException& e )
    {
      streamlog_out(WARNING) <<    _trueJetCollectionName  << " collection not available 1" << std::endl;
      tjcol = NULL;
    }

    jets=getJets();

     // get  FinalColourNeutrals
    try{
      fcncol = m_evt->getCollection( _finalColourNeutralCollectionName);
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<    _finalColourNeutralCollectionName << " collection not available" << std::endl;
        fcncol = NULL;
    }

    finalcns=getFinalcn();

     // get  InitialColourNeutrals
    try{
      icncol = m_evt->getCollection( _initialColourNeutralCollectionName);
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<    _initialColourNeutralCollectionName << " collection not available" << std::endl;
        icncol = NULL;
    }

    initialcns=getInitialcn();

     // get  FinalColourNeutralLink
    LCCollection* fcnlcol = NULL;
    try{
      fcnlcol  = m_evt->getCollection(  _finalColourNeutralLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _finalColourNeutralLink   << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    relfcn = new LCRelationNavigator( fcnlcol );

     // get  InitialColourNeutralLink
    LCCollection* icnlcol = NULL;
    try{
      icnlcol  = m_evt->getCollection(  _initialColourNeutralLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _initialColourNeutralLink  << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    relicn = new LCRelationNavigator( icnlcol );

     // get  FinalElementonLink
    LCCollection* fplcol = NULL;
    try{
      fplcol  = m_evt->getCollection(  _finalElementonLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _finalElementonLink   << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    relfp = new LCRelationNavigator( fplcol );

     // get  InitialElementonLink
    LCCollection* iplcol = NULL;
    try{
      iplcol  = m_evt->getCollection(  _initialElementonLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _initialElementonLink   << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    relip = new LCRelationNavigator( iplcol );

     // get  TrueJetPFOLink
    LCCollection* tjrecolcol = NULL;
    try{
      tjrecolcol  = m_evt->getCollection(  _trueJetPFOLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _trueJetPFOLink   << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    reltjreco = new LCRelationNavigator( tjrecolcol );


     // get  TrueJetMCParticleLink
    LCCollection* tjmcplcol = NULL;
    try{
      tjmcplcol  = m_evt->getCollection(  _trueJetMCParticleLink );
    }
    catch( lcio::DataNotAvailableException& e )
    {
        streamlog_out(WARNING) <<  _trueJetMCParticleLink   << " collection not available" << std::endl;
        fcnlcol  = NULL;
    }
    reltjmcp = new LCRelationNavigator( tjmcplcol );
    reltrue_tj =NULL ;

}
void TrueJet_Parser::delall( ) {
    if (  relfcn!= NULL ) delete relfcn;
    if (  relicn!= NULL ) delete relicn;
    if (  relfp!= NULL ) delete relfp;
    if (  relip!= NULL ) delete relip;
    if (  reltjreco != NULL) delete reltjreco;
    if (  reltjmcp != NULL) delete reltjmcp;
    if (  jets != NULL) delete  jets;
    if (  finalcns != NULL) delete  finalcns;
    if (  initialcns!= NULL ) delete   initialcns;
    if ( reltrue_tj != NULL ) delete reltrue_tj;
}
