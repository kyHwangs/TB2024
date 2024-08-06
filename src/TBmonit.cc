#include "TBmonit.h"
#include "TBmid.h"
#include "TBevt.h"
#include "TBread.h"
#include "TButility.h"

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>

#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include "TFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraph.h"

template <typename T>
TBmonit<T>::TBmonit(const std::string &fConfig_, int fRunNum_)
: fConfig(TBconfig(fConfig_)), fRunNum(fRunNum_), fMaxEvent(-1), fMaxFile(-1)
{
  fIsLive = false;
  fIsAux  = false;

  fUtility = TButility();
}

template <typename T>
TBmonit<T>::TBmonit(ObjectCollection* fObj_)
: fObj(fObj_), fConfig(TBconfig("./config_general.yml"))
{
  const YAML::Node fConfig_YAML = fConfig.GetConfig();

  std::string str_version = "";
  fObj->GetVariable("version", &str_version);

  std::string maaping_path = "../mapping/mapping_TB2024_v1.root/";
  if (str_version == "2") maaping_path = "../mapping/mapping_TB2024_v2.root/";

  fBaseDir = fConfig_YAML["BaseDirectory"].as<std::string>();
  // fMapping = fConfig_YAML["Mapping"].as<std::string>();

  fUtility = TButility(maaping_path);

  // fCaseName = fNodePlot["Name"].as<std::string>()

  fObj->GetVariable("RunNumber", &fRunNum);
  fObj->GetVariable("MaxEvent", &fMaxEvent);
  fObj->GetVariable("MaxFile", &fMaxFile);

  fObj->GetVariable("LIVE", &fIsLive);
  fObj->GetVariable("AUX", &fIsAux);

  if (fIsLive) {
    fMaxEvent = -1;
    fMaxFile = -1;
  }
}

template <typename T>
void TBmonit<T>::GetFormattedRamInfo() {

    // Total physical memory
    int64_t physical_memory;
    size_t length = sizeof(physical_memory);
    sysctlbyname("hw.memsize", &physical_memory, &length, NULL, 0);
    double total_memory_GB = static_cast<double>(physical_memory) / (1024 * 1024 * 1024);

    // Memory usage by this process
    task_basic_info_data_t info;
    mach_msg_type_number_t info_count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &info_count) == KERN_SUCCESS) {
        double process_memory_GB = static_cast<double>(info.resident_size) / (1024 * 1024 * 1024);

        // system memory usage
        vm_size_t page_size;
        mach_port_t mach_port = mach_host_self();
        vm_statistics64_data_t vm_stats;
        mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
        if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
            host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
            double free_memory_GB = static_cast<double>(vm_stats.free_count * page_size) / (1024 * 1024 * 1024);
            double used_memory_GB = total_memory_GB - free_memory_GB;


            printf("%.1f GB / %.1f GB (%0.2f %%) | Current Process: %.2f MB (%.2f %%)",
              used_memory_GB, total_memory_GB, (used_memory_GB / total_memory_GB * 100),
              process_memory_GB * 1024., (process_memory_GB / total_memory_GB * 100));
        }
    }
}

template <typename T>
void TBmonit<T>::Loop() {
  if (fIsLive) LoopLive();
  else         LoopAfterRun();
}

template <typename T>
void TBmonit<T>::LoopLive() {

  ANSI_CODE ANSI = ANSI_CODE();

  TBplotengine fPlotter = TBplotengine(fConfig.GetConfig()["ModuleConfig"], fRunNum, fIsLive, fUtility);

  std::string aCase;
  fObj->GetVariable("type", &aCase); //'single', 'heatmap'
  if (aCase == "null") {
    // !thow exception
  } else {
    fPlotter.SetCase(aCase);
  }

  std::string aMethod;
  fObj->GetVariable("method", &aMethod); //'IntADC', 'PeakADC', 'Avg', 'Overlay'
  if (aMethod == "null") {
    // !throw exception
  } else {
    fPlotter.SetMethod(aMethod);
  }

  if (aCase == "single") {
    std::vector<std::string> aModules = {};
    fObj->GetVector("module", &aModules);

    if (aModules.size() == 0) {
      // !throw exception
    } else {
      std::vector<TBcid> aCID;
      for (int i = 0; i < aModules.size(); i++)
        aCID.push_back(fUtility.GetCID(aModules.at(i)));

      fPlotter.SetCID(aCID);
    }
  }

  if (aCase == "heatmap") {
    std::vector<std::string> aModules = {};
    fObj->GetVector("module", &aModules);
    if (aModules.size() != 1) {
      // !throw exception
    } else {
      fPlotter.SetModule(aModules.at(0));
    }
  }

  fPlotter.init();

  TBread<TBwaveform> readerWave =
    TBread<TBwaveform>(
      fRunNum,
      fMaxEvent,
      fMaxFile,
      fIsLive,
      fBaseDir,
      fPlotter.GetUniqueMID()
    );

    while(1) {
      readerWave.CheckNextFileExistence();

      int iLiveCurrentEvent = readerWave.GetLiveCurrentEvent();
      int iCurrentEvent = readerWave.GetCurrentEvent();
      int iMaxEvent = readerWave.GetLiveMaxEvent();

      std::chrono::time_point time_begin = std::chrono::system_clock::now();

      for (int i = iCurrentEvent; i < iMaxEvent; i++) {

        if (i > iCurrentEvent && i % 10 == 0) {

          std::chrono::duration time_taken = std::chrono::system_clock::now() - time_begin; // delete
          float percent_done = 1. * (float)(i - iCurrentEvent) / (float)(iMaxEvent - iCurrentEvent);
          std::chrono::duration time_left = time_taken * (1 / percent_done - 1);
          std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
          std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
          std::cout << "\r\033[F" //+ ANSI.HIGHLIGHTED_GREEN + ANSI.BLACK
                    << " " << i << " / " << iMaxEvent << " events  " << minutes_left.count() << ":";
          printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);
          GetFormattedRamInfo();

          std::cout << ANSI.END << std::endl;
        }

        TBevt<TBwaveform> aEvent;
        fPlotter.Fill(readerWave.GetAnEvent());
      }
      fPlotter.Update();
    }
}


template <typename T>
void TBmonit<T>::LoopAfterRun() {

  ANSI_CODE ANSI = ANSI_CODE();

  TBplotengine fPlotter = TBplotengine(fConfig.GetConfig()["ModuleConfig"], fRunNum, fIsLive, fUtility);

  std::string aCase;
  fObj->GetVariable("type", &aCase); //'single', 'heatmap'
  if (aCase == "null") {
    // !thow exception
  } else {
    fPlotter.SetCase(aCase);
  }

  std::string aMethod;
  fObj->GetVariable("method", &aMethod); //'IntADC', 'PeakADC', 'Avg', 'Overlay'
  if (aMethod == "null") {
    // !throw exception
  } else {
    fPlotter.SetMethod(aMethod);
  }

  if (aCase == "single") {
    std::vector<std::string> aModules = {};
    fObj->GetVector("module", &aModules);

    if (aModules.size() == 0) {
      // !throw exception
    } else {
      std::vector<TBcid> aCID;
      for (int i = 0; i < aModules.size(); i++)
        aCID.push_back(fUtility.GetCID(aModules.at(i)));

      fPlotter.SetCID(aCID);
    }
  }

  if (aCase == "heatmap") {
    std::string str_version = "";
    fObj->GetVariable("version", &str_version);
    if (str_version != "1" && str_version != "2") {
      // !throw exception
    } else {
      fPlotter.SetModule(str_version);
    }
  }

  fPlotter.init();

  TBread<TBwaveform> readerWave =
    TBread<TBwaveform>(
      fRunNum,
      fMaxEvent,
      fMaxFile,
      fIsLive,
      fBaseDir,
      fPlotter.GetUniqueMID()
    );


  if (fMaxEvent == -1)
    fMaxEvent = readerWave.GetMaxEvent();

  if (fMaxEvent > readerWave.GetMaxEvent())
    fMaxEvent = readerWave.GetMaxEvent();

  std::chrono::time_point time_begin = std::chrono::system_clock::now();
  for (int i = 0; i < fMaxEvent; i++) {

    if (i > 0 && i % 10 == 0) {

      std::chrono::duration time_taken = std::chrono::system_clock::now() - time_begin; // delete
      float percent_done = 1. * (float)(i) / (float)(fMaxEvent);
      std::chrono::duration time_left = time_taken * (1 / percent_done - 1);
      std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
      std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
      std::cout << "\r\033[F" //+ ANSI.HIGHLIGHTED_GREEN + ANSI.BLACK
                << " " << i << " / " << fMaxEvent << " events  " << minutes_left.count() << ":";
      printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);
      GetFormattedRamInfo();

      std::cout << ANSI.END << std::endl;
    }

    TBevt<TBwaveform> aEvent;
    fPlotter.Fill(readerWave.GetAnEvent());
  }
  fPlotter.Update();
}


template class TBmonit<TBwaveform>;
template class TBmonit<TBfastmode>;
