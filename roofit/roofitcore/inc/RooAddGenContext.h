/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooAddGenContext.h,v 1.12 2007/05/11 09:11:30 verkerke Exp $
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef ROO_ADD_GEN_CONTEXT
#define ROO_ADD_GEN_CONTEXT

#include "RooAbsGenContext.h"
#include "RooArgSet.h"
#include <vector>
#include "RooAddPdf.h"
#include "RooAddModel.h"

class RooAddPdf;
class RooAddModel;
class RooDataSet;
class RooRealIntegral;
class RooAcceptReject;
class TRandom;

class RooAddGenContext : public RooAbsGenContext {
public:
  RooAddGenContext(const RooAddPdf &model, const RooArgSet &vars, const RooDataSet *prototype= 0,
                   const RooArgSet* auxProto=0, bool _verbose= false);
  RooAddGenContext(const RooAddModel &model, const RooArgSet &vars, const RooDataSet *prototype= 0,
                   const RooArgSet* auxProto=0, bool _verbose= false);
  ~RooAddGenContext() override;

  void setProtoDataOrder(Int_t* lut) override ;

  void attach(const RooArgSet& params) override ;

  void printMultiline(std::ostream &os, Int_t content, bool verbose=false, TString indent="") const override ;

protected:

  void initGenerator(const RooArgSet &theEvent) override;
  void generateEvent(RooArgSet &theEvent, Int_t remaining) override;
  void updateThresholds() ;

  RooAddGenContext(const RooAddGenContext& other) ;

  const RooArgSet* _vars ;
  RooArgSet* _pdfSet ;              ///<  Set owned all nodes of internal clone of p.d.f
  RooAbsPdf *_pdf ;                 ///<  Pointer to cloned p.d.f
  std::vector<RooAbsGenContext*> _gcList ;  ///<  List of component generator contexts
  Int_t  _nComp ;                   ///<  Number of PDF components
  double* _coefThresh ;           ///<[_nComp] Array of coefficient thresholds
  bool _isModel ;                 ///< Are we generating from a RooAddPdf or a RooAddModel
  RooAddModel::CacheElem* _mcache ; ///<! RooAddModel cache element
  RooAddPdf::CacheElem* _pcache ;   ///<! RooAddPdf cache element

  ClassDefOverride(RooAddGenContext,0) // Specialized context for generating a dataset from a RooAddPdf
};

#endif
