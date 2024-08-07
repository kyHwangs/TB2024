#ifndef TBplotengine_h
#define TBplotengine_h 1

#include <map>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>
#include <cmath>
#include <numeric>
#include <functional>

#include "TBconfig.h"
#include "TButility.h"
#include "TBdetector.h"
#include "TBmid.h"
#include "TBevt.h"

#include "TH1.h"
#include "TH2.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TLegend.h"

class TBplotengine
{
public:
  TBplotengine() = default;
  TBplotengine(const YAML::Node fNodePlot_, int fRunNum_, bool fLive_, TButility fUtility_);
  ~TBplotengine() {}

  enum CalcInfo
  {
    kIntADC = 0,
    kPeakADC,
    kAvgTimeStruc,
    kOverlay,
    kAux
  };

  struct PlotInfo {
    TBcid cid;
    std::string name;
    TButility::mod_info info;

    TH2D* hist2D;
    TH1D* hist1D;

    int xInit;
    int xFin;

    PlotInfo(TBcid cid_, std::string name_, TButility::mod_info info_)
    : cid(cid_), name(name_), info(info_), xInit(0), xFin(0)
    {}

    PlotInfo(TBcid cid_, std::string name_, TButility::mod_info info_, int xInit_, int xFin_)
    : cid(cid_), name(name_), info(info_), xInit(xInit_), xFin(xFin_)
    {}

    void SetPlot(TH1D* aHist) { hist1D = aHist; }
    void SetPlot(TH2D* aHist) { hist2D = aHist; }
  };

  void init();
  void init_2D();
  void init_MCPPMT();
  void init_SiPM();
  void PrintInfo();

  void Fill(TBevt<TBwaveform> anEvent);
  void Fill(TBevt<TBfastmode> anEvent) {}

  void Draw();
  void Update();
  void SetMaximum();

  void SaveAs(TString output);

  double GetPeakADC(std::vector<short> waveform, int xInit, int xFil);
  double GetIntADC(std::vector<short> waveform, int xInit, int xFil);

  double GetValue(std::vector<short> waveform, int xInit, int xFil) {

    if(fCalcInfo == CalcInfo::kPeakADC)
      return GetPeakADC(waveform, xInit, xFil);

    if(fCalcInfo == CalcInfo::kIntADC)
      return GetIntADC(waveform, xInit, xFil);

    return -999;
  }

  std::vector<int> GetUniqueMID();

  void SetCID(std::vector<TBcid> cids) { fCIDtoPlot_Ceren = cids; }
  // void SetCID(std::string cases) { SetCID(fUtility.loadCID(cases)); }
  void SetCase(std::string cases) { fCaseName = cases; }
  void SetModule(std::string module) { fModule = module; }

  void SetMethod(std::string fMethod) {
    if (fMethod == "IntADC")
      fCalcInfo = kIntADC;

    if (fMethod == "PeakADC")
      fCalcInfo = kPeakADC;

    if (fMethod == "Avg")
      fCalcInfo = kAvgTimeStruc;

    if (fMethod == "Overlay")
      fCalcInfo = kOverlay;

    if (fMethod == "AUX")
      fCalcInfo = kAux;
  }

private:
  const YAML::Node fConfig;
  int fRunNum;
  TButility fUtility;

  bool fIsFirst;
  bool fLive;

  TApplication* fApp;
  TCanvas* fCanvas;
  TLegend* fLeg;

  CalcInfo fCalcInfo;

  std::string fCaseName;
  std::string fModule;

  TH1D* fMainFrame;
  
  TH2D* f2DHistCeren;
  TH2D* f2DHistScint;

  std::vector<TBcid> fCIDtoPlot_Ceren;
  std::vector<TBcid> fCIDtoPlot_Scint;

  std::vector<PlotInfo> fPlotter_Ceren;
  std::vector<PlotInfo> fPlotter_Scint;

};

#endif
