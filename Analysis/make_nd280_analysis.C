#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <TStyle.h>
#include <TDirectory.h>
#include <TMath.h>
#include <TString.h>
#include <TParameter.h>
#include <TObjString.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>

static std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> out; std::string field; bool inQuotes=false;
    for (size_t i=0;i<line.size();++i) {
        char c=line[i];
        if(c=='"') { if(inQuotes && i+1<line.size() && line[i+1]=='"'){field.push_back('"');++i;} else inQuotes=!inQuotes; }
        else if(c==',' && !inQuotes){out.push_back(field);field.clear();}
        else field.push_back(c);
    }
    out.push_back(field); return out;
}

void make_nd280_analysis(const char* inputCsv="nd280_2026-07-15_19-09-58.csv",
                         const char* outputRoot="nd280_2026-07-15_19-09-58_analysis.root",
                         const char* plotDir="nd280_plots") {
    std::ifstream input(inputCsv);
    if(!input.is_open()){ std::cerr << "Cannot open " << inputCsv << std::endl; return; }
    std::string header; std::getline(input,header);

    char host_time_iso[64]={0}, firmware[64]={0}, status_hex[32]={0};
    double elapsed_s=0, temperature_c=0, bx_mt=0, by_mt=0, bz_mt=0, bmag_mt=0;
    int board_id=0, average=0; Long64_t sequence=0;
    std::vector<double> vt,vtemp,vbx,vby,vbz,vbmag;

    std::string line; Long64_t nRows=0,nSkipped=0;
    while(std::getline(input,line)){
        if(line.empty()) continue; auto f=splitCSV(line); if(f.size()<12){++nSkipped;continue;}
        try {
            elapsed_s=std::stod(f[1]); board_id=std::stoi(f[3]); sequence=std::stoll(f[4]); average=std::stoi(f[5]);
            temperature_c=std::stod(f[6]); bx_mt=std::stod(f[7]); by_mt=std::stod(f[8]); bz_mt=std::stod(f[9]);
        } catch(...){++nSkipped;continue;}
        bmag_mt=std::sqrt(bx_mt*bx_mt+by_mt*by_mt+bz_mt*bz_mt);
        vt.push_back(elapsed_s); vtemp.push_back(temperature_c); vbx.push_back(bx_mt); vby.push_back(by_mt); vbz.push_back(bz_mt); vbmag.push_back(bmag_mt);
        ++nRows;
    }
    input.close();
    if(nRows==0){std::cerr<<"No valid rows"<<std::endl;return;}

    auto mm=[](const std::vector<double>& v){return std::minmax_element(v.begin(),v.end());};
    auto tx=mm(vtemp), bx=mm(vbx), by=mm(vby), bz=mm(vbz), bm=mm(vbmag);
    auto pad=[](double lo,double hi){double d=hi-lo; if(d<=0)d=1; return std::pair<double,double>(lo-0.05*d,hi+0.05*d);};
    auto tr=pad(*tx.first,*tx.second), bxr=pad(*bx.first,*bx.second), byr=pad(*by.first,*by.second), bzr=pad(*bz.first,*bz.second), bmr=pad(*bm.first,*bm.second);

    TFile *fout=TFile::Open(outputRoot,"RECREATE"); if(!fout||fout->IsZombie()){std::cerr<<"Cannot create ROOT file"<<std::endl;return;}
    TTree *tree=new TTree("measurements","ND280 TMAG magnetic field measurements");
    tree->Branch("host_time_iso",host_time_iso,"host_time_iso/C"); tree->Branch("elapsed_s",&elapsed_s,"elapsed_s/D");
    tree->Branch("firmware",firmware,"firmware/C"); tree->Branch("board_id",&board_id,"board_id/I"); tree->Branch("sequence",&sequence,"sequence/L");
    tree->Branch("average",&average,"average/I"); tree->Branch("temperature_c",&temperature_c,"temperature_c/D");
    tree->Branch("bx_mt",&bx_mt,"bx_mt/D"); tree->Branch("by_mt",&by_mt,"by_mt/D"); tree->Branch("bz_mt",&bz_mt,"bz_mt/D");
    tree->Branch("bmag_mt",&bmag_mt,"bmag_mt/D"); tree->Branch("status_hex",status_hex,"status_hex/C");

    // Re-read to preserve string branches exactly.
    input.open(inputCsv); std::getline(input,header);
    while(std::getline(input,line)){
        if(line.empty())continue; auto f=splitCSV(line); if(f.size()<12)continue;
        try { std::strncpy(host_time_iso,f[0].c_str(),63);host_time_iso[63]='\0'; elapsed_s=std::stod(f[1]); std::strncpy(firmware,f[2].c_str(),63);firmware[63]='\0'; board_id=std::stoi(f[3]); sequence=std::stoll(f[4]); average=std::stoi(f[5]); temperature_c=std::stod(f[6]); bx_mt=std::stod(f[7]); by_mt=std::stod(f[8]); bz_mt=std::stod(f[9]); std::strncpy(status_hex,f[10].c_str(),31);status_hex[31]='\0'; }
        catch(...){continue;} bmag_mt=std::sqrt(bx_mt*bx_mt+by_mt*by_mt+bz_mt*bz_mt); tree->Fill();
    }
    input.close();

    TDirectory *dHist=fout->mkdir("histograms"); dHist->cd();
    TH1D *hBx=new TH1D("hBx","B_{x} distribution;B_{x} [mT];Entries",200,bxr.first,bxr.second);
    TH1D *hBy=new TH1D("hBy","B_{y} distribution;B_{y} [mT];Entries",200,byr.first,byr.second);
    TH1D *hBz=new TH1D("hBz","B_{z} distribution;B_{z} [mT];Entries",200,bzr.first,bzr.second);
    TH1D *hBmag=new TH1D("hBmag","|B| distribution;|B| [mT];Entries",200,bmr.first,bmr.second);
    TH1D *hTemperature=new TH1D("hTemperature","Temperature distribution;Temperature [^{o}C];Entries",120,tr.first,tr.second);
    for(size_t i=0;i<vbx.size();++i){hBx->Fill(vbx[i]);hBy->Fill(vby[i]);hBz->Fill(vbz[i]);hBmag->Fill(vbmag[i]);hTemperature->Fill(vtemp[i]);}

    TDirectory *dCorr=fout->mkdir("temperature_correlations"); dCorr->cd();
    TH2D *hBxT=new TH2D("hBx_vs_T","B_{x} vs temperature;Temperature [^{o}C];B_{x} [mT]",80,tr.first,tr.second,120,bxr.first,bxr.second);
    TH2D *hByT=new TH2D("hBy_vs_T","B_{y} vs temperature;Temperature [^{o}C];B_{y} [mT]",80,tr.first,tr.second,120,byr.first,byr.second);
    TH2D *hBzT=new TH2D("hBz_vs_T","B_{z} vs temperature;Temperature [^{o}C];B_{z} [mT]",80,tr.first,tr.second,120,bzr.first,bzr.second);
    TH2D *hBmagT=new TH2D("hBmag_vs_T","|B| vs temperature;Temperature [^{o}C];|B| [mT]",80,tr.first,tr.second,120,bmr.first,bmr.second);
    TProfile *pBxT=new TProfile("pBx_vs_T","Mean B_{x} vs temperature;Temperature [^{o}C];Mean B_{x} [mT]",40,tr.first,tr.second);
    TProfile *pByT=new TProfile("pBy_vs_T","Mean B_{y} vs temperature;Temperature [^{o}C];Mean B_{y} [mT]",40,tr.first,tr.second);
    TProfile *pBzT=new TProfile("pBz_vs_T","Mean B_{z} vs temperature;Temperature [^{o}C];Mean B_{z} [mT]",40,tr.first,tr.second);
    TProfile *pBmagT=new TProfile("pBmag_vs_T","Mean |B| vs temperature;Temperature [^{o}C];Mean |B| [mT]",40,tr.first,tr.second);
    for(size_t i=0;i<vbx.size();++i){hBxT->Fill(vtemp[i],vbx[i]);hByT->Fill(vtemp[i],vby[i]);hBzT->Fill(vtemp[i],vbz[i]);hBmagT->Fill(vtemp[i],vbmag[i]);pBxT->Fill(vtemp[i],vbx[i]);pByT->Fill(vtemp[i],vby[i]);pBzT->Fill(vtemp[i],vbz[i]);pBmagT->Fill(vtemp[i],vbmag[i]);}
    TF1 *fBxT=new TF1("fit_Bx_vs_T","pol1",tr.first,tr.second); TF1 *fByT=new TF1("fit_By_vs_T","pol1",tr.first,tr.second); TF1 *fBzT=new TF1("fit_Bz_vs_T","pol1",tr.first,tr.second); TF1 *fBmagT=new TF1("fit_Bmag_vs_T","pol1",tr.first,tr.second);
    pBxT->Fit(fBxT,"Q"); pByT->Fit(fByT,"Q"); pBzT->Fit(fBzT,"Q"); pBmagT->Fit(fBmagT,"Q");

    TDirectory *dGraphs=fout->mkdir("graphs"); dGraphs->cd();
    TGraph *gBxT=new TGraph(nRows,vtemp.data(),vbx.data()); gBxT->SetName("gBx_vs_T"); gBxT->SetTitle("B components vs temperature;Temperature [^{o}C];B [mT]");
    TGraph *gByT=new TGraph(nRows,vtemp.data(),vby.data()); gByT->SetName("gBy_vs_T");
    TGraph *gBzT=new TGraph(nRows,vtemp.data(),vbz.data()); gBzT->SetName("gBz_vs_T");
    TGraph *gBxTime=new TGraph(nRows,vt.data(),vbx.data()); gBxTime->SetName("gBx_vs_time");
    TGraph *gByTime=new TGraph(nRows,vt.data(),vby.data()); gByTime->SetName("gBy_vs_time");
    TGraph *gBzTime=new TGraph(nRows,vt.data(),vbz.data()); gBzTime->SetName("gBz_vs_time");
    gBxT->SetMarkerColor(kRed);gByT->SetMarkerColor(kBlue);gBzT->SetMarkerColor(kGreen+2); gBxT->SetMarkerStyle(20);gByT->SetMarkerStyle(20);gBzT->SetMarkerStyle(20);gBxT->SetMarkerSize(.25);gByT->SetMarkerSize(.25);gBzT->SetMarkerSize(.25);
    gBxTime->SetLineColor(kRed);gByTime->SetLineColor(kBlue);gBzTime->SetLineColor(kGreen+2);

    TDirectory *dCan=fout->mkdir("canvases"); dCan->cd();
    TCanvas *cBT=new TCanvas("c_B_components_vs_T","B components vs temperature",1200,750); gBxT->Draw("AP");gByT->Draw("P SAME");gBzT->Draw("P SAME");
    TLegend *l1=new TLegend(.80,.75,.93,.90);l1->AddEntry(gBxT,"B_{x}","p");l1->AddEntry(gByT,"B_{y}","p");l1->AddEntry(gBzT,"B_{z}","p");l1->Draw();cBT->SetGrid();
    TCanvas *cProf=new TCanvas("c_profiles_vs_T","Mean B vs temperature",1200,750); pBxT->SetLineColor(kRed);pByT->SetLineColor(kBlue);pBzT->SetLineColor(kGreen+2);pBxT->SetMarkerColor(kRed);pByT->SetMarkerColor(kBlue);pBzT->SetMarkerColor(kGreen+2);pBxT->SetMarkerStyle(20);pByT->SetMarkerStyle(21);pBzT->SetMarkerStyle(22);pBxT->SetTitle("Mean magnetic field vs temperature;Temperature [^{o}C];Mean B [mT]");pBxT->Draw();pByT->Draw("SAME");pBzT->Draw("SAME");TLegend *l2=new TLegend(.80,.75,.93,.90);l2->AddEntry(pBxT,"B_{x}","lp");l2->AddEntry(pByT,"B_{y}","lp");l2->AddEntry(pBzT,"B_{z}","lp");l2->Draw();cProf->SetGrid();
    TCanvas *cTime=new TCanvas("c_B_components_vs_time","B components vs time",1200,750);gBxTime->SetTitle("Magnetic field stability;Elapsed time [s];B [mT]");gBxTime->Draw("AL");gByTime->Draw("L SAME");gBzTime->Draw("L SAME");TLegend *l3=new TLegend(.80,.75,.93,.90);l3->AddEntry(gBxTime,"B_{x}","l");l3->AddEntry(gByTime,"B_{y}","l");l3->AddEntry(gBzTime,"B_{z}","l");l3->Draw();cTime->SetGrid();

    gSystem->mkdir(plotDir,true); cBT->SaveAs(Form("%s/B_components_vs_temperature.pdf",plotDir)); cProf->SaveAs(Form("%s/B_profiles_vs_temperature.pdf",plotDir)); cTime->SaveAs(Form("%s/B_components_vs_time.pdf",plotDir));

    fout->cd(); tree->Write(); TParameter<Long64_t>("rows_written",nRows).Write(); TParameter<Long64_t>("rows_skipped",nSkipped).Write();
    TParameter<double>("Bx_temp_slope_mT_per_C",fBxT->GetParameter(1)).Write(); TParameter<double>("By_temp_slope_mT_per_C",fByT->GetParameter(1)).Write(); TParameter<double>("Bz_temp_slope_mT_per_C",fBzT->GetParameter(1)).Write(); TParameter<double>("Bmag_temp_slope_mT_per_C",fBmagT->GetParameter(1)).Write();
    TObjString(header.c_str()).Write("csv_header"); fout->Write(); fout->Close();
    std::cout<<"Created "<<outputRoot<<" with "<<nRows<<" rows (skipped "<<nSkipped<<")\n";
    std::cout<<"Temperature slopes [mT/C]: Bx="<<fBxT->GetParameter(1)<<" By="<<fByT->GetParameter(1)<<" Bz="<<fBzT->GetParameter(1)<<" |B|="<<fBmagT->GetParameter(1)<<std::endl;
}
