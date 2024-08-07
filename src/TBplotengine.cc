#include "TBplotengine.h"
#include "GuiTypes.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TPaveStats.h"

TBplotengine::TBplotengine(const YAML::Node fConfig_, int fRunNum_, bool fLive_, TButility fUtility_)
: fConfig(fConfig_), fRunNum(fRunNum_), fLive(fLive_), fUtility(fUtility_), fCaseName("")
{}

void TBplotengine::init() {

  fIsFirst = true;

  if (fCaseName == "single") {

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC) {
      gStyle->SetPalette(kVisibleSpectrum);
      fMainFrame = new TH1D("fMainFrame", ";IntADC;nEvents", 220, -3000., 30000.);
      fMainFrame->SetStats(0);
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
      gStyle->SetPalette(kVisibleSpectrum);
      fMainFrame = new TH1D("fMainFrame", ";PeakADC;nEvents", 288, -512., 4096.);
      fMainFrame->SetStats(0);
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
      gStyle->SetPalette(kVisibleSpectrum);
      fMainFrame = new TH1D("fMainFrame", ";Bin;ADC", 1000, 0.5, 1000.5);
      fMainFrame->SetStats(0);
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
      fLeg = new TLegend(0.7, 0.2, 0.9, 0.5);

    for (int i = 0; i < fCIDtoPlot_Ceren.size(); i++) {
      TBcid aCID = fCIDtoPlot_Ceren.at(i);
      std::string aName = fUtility.GetName(aCID);
      TButility::mod_info aInfo = fUtility.GetInfo(aCID);
      // std::cout << aName << " "; aCID.print();

      if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
        std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
          fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 220, -3000., 30000.));

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 288, -512., 4096.));

        fPlotter_Ceren.at(i).hist1D->SetLineColor(
          gStyle->GetColorPalette((float)(i + 1) * ((float)gStyle->GetNumberOfColors() / ((float)fCIDtoPlot_Ceren.size() + 1)))
        );
        fPlotter_Ceren.at(i).hist1D->SetLineWidth(4);

      } else if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, 0, 0));
        fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";Bin;ADC", 1000, 0.5, 1000.5));
        fPlotter_Ceren.at(i).hist1D->SetLineColor(
          gStyle->GetColorPalette((float)(i + 1) * ((float)gStyle->GetNumberOfColors() / ((float)fCIDtoPlot_Ceren.size() + 1)))
        );
        fPlotter_Ceren.at(i).hist1D->SetLineWidth(2);
        fPlotter_Ceren.at(i).hist1D->SetStats(0);

      } else if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, 0, 0));
        fPlotter_Ceren.at(i).SetPlot(new TH2D((TString)(aName), ";Bin;ADC", 1024, 0., 1024., 4096, 0., 4096.));
        fPlotter_Ceren.at(i).hist2D->SetStats(0);

      } else {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo));
      }
    }

    int argc = 0;
    char* argv[] = {};
    fApp = new TApplication("app", &argc, argv);
    if (fLive)
      fApp->SetReturnFromRun(true);

    fCanvas = new TCanvas("", "", 1000, 1000);

    Draw();
  } else if (fCaseName == "heatmap") {

    int argc = 0;
    char* argv[] = {};
    fApp = new TApplication("app", &argc, argv);

    if (fLive)
      fApp->SetReturnFromRun(true);

    fCanvas = new TCanvas("", "", 1900, 1000);
    fCanvas->Divide(2, 1);

    auto tPadLeft = fCanvas->cd(1);
    tPadLeft->SetRightMargin(0.13);

    auto tPadRight = fCanvas->cd(2);
    tPadRight->SetRightMargin(0.13);

    init_2D();
  }

  // std::cout << fCIDtoPlot_Ceren.size() << std::endl;
  // for (int i = 0; i < fPlotter_Ceren.size(); i++) {
  //   std::cout << i << " " << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << " ";
  //   fPlotter_Ceren.at(i).cid.print();
  // }

  // std::cout << fCIDtoPlot_Scint.size() << std::endl;
  // for (int i = 0; i < fPlotter_Scint.size(); i++) {
  //   std::cout << i << " " << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << " ";
  //   fPlotter_Scint.at(i).cid.print();
  // }
}

void TBplotengine::init_2D() {

  int nModule = 15;
  if (fModule == "1") nModule = 15;
  if (fModule == "2") nModule = 16;

  for (int i = 1; i <= nModule; i++) {

    std::string aLName = "L" + std::to_string(i);
    TBcid aLCID = fUtility.GetCID(aLName);
    TButility::mod_info aLInfo = fUtility.GetInfo(aLCID);

    // std::cout << i << " " << aLName << " " << aLInfo.row << " " << aLInfo.col << std::endl;

    fCIDtoPlot_Ceren.push_back(aLCID);

    std::vector<int> rinterval = fConfig[aLName].as<std::vector<int>>();
    fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aLCID, aLName, aLInfo, rinterval.at(0), rinterval.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Ceren.at(fPlotter_Ceren.size() - 1).SetPlot(new TH1D((TString)(aLName), ";IntADC;nEvents", 220, -3000., 30000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Ceren.at(fPlotter_Ceren.size() - 1).SetPlot(new TH1D((TString)(aLName), ";IntADC;nEvents", 288, -512., 4096.));


    std::string aRName = "R" + std::to_string(i);
    TBcid aRCID = fUtility.GetCID(aRName);
    TButility::mod_info aRInfo = fUtility.GetInfo(aRCID);

    // std::cout << i << " " << aRName << " " << aRInfo.row << " " << aRInfo.col << std::endl;

    fCIDtoPlot_Scint.push_back(aRCID);

    std::vector<int> linterval = fConfig[aRName].as<std::vector<int>>();
    fPlotter_Scint.push_back(TBplotengine::PlotInfo(aRCID, aRName, aRInfo, linterval.at(0), linterval.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Scint.at(fPlotter_Scint.size() - 1).SetPlot(new TH1D((TString)(aRName), ";IntADC;nEvents", 220, -3000., 30000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Scint.at(fPlotter_Scint.size() - 1).SetPlot(new TH1D((TString)(aRName), ";IntADC;nEvents", 288, -512., 4096.));

  }

  if (fModule == "1") {
    f2DHistCeren = new TH2D("Left", "Left;;", 5, 0.5, 5.5, 3, 0.5, 3.5);
    f2DHistCeren->SetStats(0);

    f2DHistScint = new TH2D("Right", "Right;;", 5, 0.5, 5.5, 3, 0.5, 3.5);
    f2DHistScint->SetStats(0);

    for (int i = 1; i <= 5; i++) {
      f2DHistCeren->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
      f2DHistScint->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    }

    for (int i = 1; i <= 3; i++) {
      f2DHistCeren->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
      f2DHistScint->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    }
  } else {
    f2DHistCeren = new TH2D("Left", "Left;;", 4, 0.5, 4.5, 4, 0.5, 4.5);
    f2DHistCeren->SetStats(0);

    f2DHistScint = new TH2D("Right", "Right;;", 4, 0.5, 4.5, 4, 0.5, 4.5);
    f2DHistScint->SetStats(0);

    for (int i = 1; i <= 4; i++) {
      f2DHistCeren->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
      f2DHistScint->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    }

    for (int i = 1; i <= 4; i++) {
      f2DHistCeren->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
      f2DHistScint->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    }
  }

  // std::cout << fCIDtoPlot_Ceren.size() << std::endl;
  // for (int i = 0; i < fPlotter_Ceren.size(); i++) {
  //   std::cout << i << " " << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << " ";
  //   fPlotter_Ceren.at(i).cid.print();
  // }

  // std::cout << fCIDtoPlot_Scint.size() << std::endl;
  // for (int i = 0; i < fPlotter_Scint.size(); i++) {
  //   std::cout << i << " " << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << " ";
  //   fPlotter_Scint.at(i).cid.print();
  // }

  Draw();
}


double TBplotengine::GetPeakADC(std::vector<short> waveform, int xInit, int xFin) {
  double ped = 0;
  for (int i = 1; i < 101; i++)
    ped += (double)waveform.at(i) / 100.;

  std::vector<double> pedCorWave;
  for (int i = xInit; i < xFin; i++)
    pedCorWave.push_back(ped - (double)waveform.at(i));

  return *std::max_element(pedCorWave.begin(), pedCorWave.end());
}

double TBplotengine::GetIntADC(std::vector<short> waveform, int xInit, int xFin) {
  double ped = 0;
  for (int i = 1; i < 101; i++)
    ped += (double)waveform.at(i) / 100.;

  double intADC_ = 0;
  for (int i = xInit; i < xFin; i++)
    intADC_ += ped - (double)waveform.at(i);

  return intADC_;
}


void TBplotengine::PrintInfo() {

}

void TBplotengine::Fill(TBevt<TBwaveform> anEvent) {

  if (fCaseName == "single") {
    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        double value = GetValue(anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform(), fPlotter_Ceren.at(i).xInit, fPlotter_Ceren.at(i).xFin);
        fPlotter_Ceren.at(i).hist1D->Fill(value);
      }
    } else if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        auto tWave = anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform();
        for (int j = 1; j <= 1000; j++) {
          fPlotter_Ceren.at(i).hist1D->Fill(j, tWave.at(j));
        }
        fPlotter_Ceren.at(i).xInit++;
      }
    } else if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        auto tWave = anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform();
        for (int j = 0; j < tWave.size(); j++) {
          fPlotter_Ceren.at(i).hist2D->Fill(j, tWave.at(j));
        }
      }
    }

  } else if (fCaseName == "heatmap") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      double value = GetValue(anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform(), fPlotter_Ceren.at(i).xInit, fPlotter_Ceren.at(i).xFin);
      fPlotter_Ceren.at(i).hist1D->Fill(value);
    }

    for (int i = 0; i < fPlotter_Scint.size(); i++) {
      double value = GetValue(anEvent.GetData(fPlotter_Scint.at(i).cid).waveform(), fPlotter_Scint.at(i).xInit, fPlotter_Scint.at(i).xFin);
      fPlotter_Scint.at(i).hist1D->Fill(value);
    }
  }
}

void TBplotengine::Draw() {
;
  if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      fLeg->AddEntry(fPlotter_Ceren.at(i).hist1D, fPlotter_Ceren.at(i).name.c_str(), "l");

  }

  fCanvas->cd();
  if (fCaseName == "single") {
    if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
      fPlotter_Ceren.at(0).hist2D->Draw("colz");

    } else {
      fMainFrame->Draw("");
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist1D->Draw("Hist & same");

      if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
        fLeg->Draw("same");
    }

  } else if (fCaseName == "heatmap") {
    fCanvas->cd(1);
    f2DHistCeren->Draw("colz text");

    fCanvas->cd(2);
    f2DHistScint->Draw("colz text");
  }

  gSystem->ProcessEvents();
  gSystem->Sleep(3000);
}

void TBplotengine::Update() {

  if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      fPlotter_Ceren.at(i).hist1D->Scale(1./(float)fPlotter_Ceren.at(i).xInit);

  if (fCaseName == "single") {
    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC || fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
      SetMaximum();

    if (fIsFirst) {
      fIsFirst = false;
      if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
        fPlotter_Ceren.at(0).hist2D->Draw("colz");

      } else {
        fMainFrame->Draw("");

        double stat_height = (1.0 - 0.2)  / (double)fPlotter_Ceren.size();
        for (int i = 0; i < fPlotter_Ceren.size(); i++) {
          fPlotter_Ceren.at(i).hist1D->Draw("Hist & sames");

          if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
            fCanvas->Update();
            // TPaveStats* stat = (TPaveStats*)fCanvas->GetPrimitive("stats");
            TPaveStats* stat = (TPaveStats*)fPlotter_Ceren.at(i).hist1D->FindObject("stats");
            // stat->SetName(fPlotter_Ceren.at(i).hist1D->GetName() + (TString)"_stat");
            stat->SetTextColor(fPlotter_Ceren.at(i).hist1D->GetLineColor());
            stat->SetY2NDC(1. - stat_height * i);
            stat->SetY1NDC(1. - stat_height * (i + 1));
            stat->SaveStyle();
          }
        }
        
        if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
          fLeg->Draw("same");
      }
    }
  } else if (fCaseName == "heatmap") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      f2DHistCeren->SetBinContent(fPlotter_Ceren.at(i).info.row, fPlotter_Ceren.at(i).info.col, fPlotter_Ceren.at(i).hist1D->GetMean());
      // std::cout << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << std::endl;
    }

    for (int i = 0; i < fPlotter_Scint.size(); i++) {
      f2DHistScint->SetBinContent(fPlotter_Scint.at(i).info.row, fPlotter_Scint.at(i).info.col, fPlotter_Scint.at(i).hist1D->GetMean());
      // std::cout << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << std::endl;
    }

  }

  fCanvas->Update();
  fCanvas->Pad()->Draw();

  // gSystem->ProcessEvents();

  if (!fLive) {
    fApp->Run(true);
  } else {
    gSystem->ProcessEvents();
  }

  if (fLive)
    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist1D->Scale((float)fPlotter_Ceren.at(i).xInit);

  gSystem->Sleep(5000);

}

void TBplotengine::SetMaximum() {

  float max = -999;
  for (int i = 0; i < fPlotter_Ceren.size(); i++) {
    if (max < fPlotter_Ceren.at(i).hist1D->GetMaximum()) {
      max = fPlotter_Ceren.at(i).hist1D->GetMaximum();
    }
  }

  fMainFrame->GetYaxis()->SetRangeUser(0., max * 1.2);
}

void TBplotengine::SaveAs(TString output = "")
{
  if (output == "")
    output = std::to_string(fRunNum) + "_DQMoutput.root";

  // output = (TString)("./ROOT/"+ output);
  // if (fCaseName != "")
  //   output = (TString)(output + "_" + fCaseName);

  TFile* outoutFile = new TFile(output, "RECREATE");

  outoutFile->cd();

  for (int i = 0; i < fPlotter_Ceren.size(); i++)
    fPlotter_Ceren.at(i).hist1D->Write();

  for (int i = 0; i < fPlotter_Scint.size(); i++)
    fPlotter_Scint.at(i).hist1D->Write();

  outoutFile->Close();
}

std::vector<int> TBplotengine::GetUniqueMID() {
  if (fCaseName == "single") {

    return fUtility.GetUniqueMID(fCIDtoPlot_Ceren);
  } else if (fCaseName == "heatmap") {

    return fUtility.GetUniqueMID(fCIDtoPlot_Ceren, fCIDtoPlot_Scint);
  }

  return std::vector<int>{};
}
