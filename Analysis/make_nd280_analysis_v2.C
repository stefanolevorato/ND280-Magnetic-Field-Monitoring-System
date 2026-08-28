#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <TDirectory.h>
#include <TString.h>
#include <TParameter.h>
#include <TObjString.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TPad.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <utility>

static std::vector<std::string> splitCSV(const std::string& line)
{
    std::vector<std::string> out;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            out.push_back(field);
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    out.push_back(field);
    return out;
}

static std::pair<double,double> paddedRange(double lo, double hi, double fraction = 0.05)
{
    double d = hi - lo;
    if (d <= 0.0) d = 1.0;
    return {lo - fraction * d, hi + fraction * d};
}

void make_nd280_analysis_v2(
    const char* inputCsv  = "nd280_2026-07-15_19-09-58.csv",
    const char* outputRoot = "nd280_2026-07-15_19-09-58_analysis.root",
    const char* plotDir = "nd280_plots")
{
    gStyle->SetOptStat(1110);

    // ------------------------------------------------------------------
    // First pass: read numerical data and determine plotting ranges.
    // ------------------------------------------------------------------
    std::ifstream input(inputCsv);
    if (!input.is_open()) {
        std::cerr << "Cannot open input file: " << inputCsv << std::endl;
        return;
    }

    std::string header;
    std::getline(input, header);

    std::vector<double> vt;
    std::vector<double> vtemp;
    std::vector<double> vbx;
    std::vector<double> vby;
    std::vector<double> vbz;
    std::vector<double> vbmag;

    std::string line;
    Long64_t nRows = 0;
    Long64_t nSkipped = 0;

    while (std::getline(input, line)) {
        if (line.empty()) continue;

        const auto f = splitCSV(line);
        if (f.size() < 11) {
            ++nSkipped;
            continue;
        }

        try {
            const double elapsed_s = std::stod(f[1]);
            const double temperature_c = std::stod(f[6]);
            const double bx_mt = std::stod(f[7]);
            const double by_mt = std::stod(f[8]);
            const double bz_mt = std::stod(f[9]);
            const double bmag_mt = std::sqrt(bx_mt*bx_mt + by_mt*by_mt + bz_mt*bz_mt);

            vt.push_back(elapsed_s);
            vtemp.push_back(temperature_c);
            vbx.push_back(bx_mt);
            vby.push_back(by_mt);
            vbz.push_back(bz_mt);
            vbmag.push_back(bmag_mt);
            ++nRows;
        }
        catch (...) {
            ++nSkipped;
        }
    }
    input.close();

    if (nRows == 0) {
        std::cerr << "No valid rows found in " << inputCsv << std::endl;
        return;
    }

    auto minmax = [](const std::vector<double>& v) {
        return std::minmax_element(v.begin(), v.end());
    };

    const auto mmT  = minmax(vtemp);
    const auto mmBx = minmax(vbx);
    const auto mmBy = minmax(vby);
    const auto mmBz = minmax(vbz);
    const auto mmBm = minmax(vbmag);
    const auto mmTime = minmax(vt);

    const auto tr  = paddedRange(*mmT.first,  *mmT.second);
    const auto bxr = paddedRange(*mmBx.first, *mmBx.second);
    const auto byr = paddedRange(*mmBy.first, *mmBy.second);
    const auto bzr = paddedRange(*mmBz.first, *mmBz.second);
    const auto bmr = paddedRange(*mmBm.first, *mmBm.second);
    const auto timer = paddedRange(*mmTime.first, *mmTime.second, 0.01);

    // Common Y range for Bx, By and Bz overlays.
    const double allBmin = std::min({*mmBx.first, *mmBy.first, *mmBz.first});
    const double allBmax = std::max({*mmBx.second, *mmBy.second, *mmBz.second});
    const auto allBr = paddedRange(allBmin, allBmax, 0.10);

    std::cout << "Rows read: " << nRows << "   skipped: " << nSkipped << std::endl;
    std::cout << "Temperature range: " << *mmT.first << " to " << *mmT.second << " C" << std::endl;

    // ------------------------------------------------------------------
    // Output ROOT file and TTree.
    // ------------------------------------------------------------------
    TFile* fout = TFile::Open(outputRoot, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "Cannot create ROOT file: " << outputRoot << std::endl;
        return;
    }

    char host_time_iso[64] = {0};
    char firmware[64] = {0};
    char status_hex[32] = {0};
    double elapsed_s = 0.0;
    double temperature_c = 0.0;
    double bx_mt = 0.0;
    double by_mt = 0.0;
    double bz_mt = 0.0;
    double bmag_mt = 0.0;
    int board_id = 0;
    int average = 0;
    Long64_t sequence = 0;

    TTree* tree = new TTree("measurements", "ND280 TMAG magnetic field measurements");
    tree->Branch("host_time_iso", host_time_iso, "host_time_iso/C");
    tree->Branch("elapsed_s", &elapsed_s, "elapsed_s/D");
    tree->Branch("firmware", firmware, "firmware/C");
    tree->Branch("board_id", &board_id, "board_id/I");
    tree->Branch("sequence", &sequence, "sequence/L");
    tree->Branch("average", &average, "average/I");
    tree->Branch("temperature_c", &temperature_c, "temperature_c/D");
    tree->Branch("bx_mt", &bx_mt, "bx_mt/D");
    tree->Branch("by_mt", &by_mt, "by_mt/D");
    tree->Branch("bz_mt", &bz_mt, "bz_mt/D");
    tree->Branch("bmag_mt", &bmag_mt, "bmag_mt/D");
    tree->Branch("status_hex", status_hex, "status_hex/C");

    // Second pass keeps string fields exactly as recorded in the CSV.
    input.open(inputCsv);
    std::getline(input, header);
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        const auto f = splitCSV(line);
        if (f.size() < 11) continue;

        try {
            std::strncpy(host_time_iso, f[0].c_str(), sizeof(host_time_iso)-1);
            host_time_iso[sizeof(host_time_iso)-1] = '\0';
            elapsed_s = std::stod(f[1]);

            std::strncpy(firmware, f[2].c_str(), sizeof(firmware)-1);
            firmware[sizeof(firmware)-1] = '\0';

            board_id = std::stoi(f[3]);
            sequence = std::stoll(f[4]);
            average = std::stoi(f[5]);
            temperature_c = std::stod(f[6]);
            bx_mt = std::stod(f[7]);
            by_mt = std::stod(f[8]);
            bz_mt = std::stod(f[9]);

            std::strncpy(status_hex, f[10].c_str(), sizeof(status_hex)-1);
            status_hex[sizeof(status_hex)-1] = '\0';
        }
        catch (...) {
            continue;
        }

        bmag_mt = std::sqrt(bx_mt*bx_mt + by_mt*by_mt + bz_mt*bz_mt);
        tree->Fill();
    }
    input.close();

    // ------------------------------------------------------------------
    // 1D distributions.
    // ------------------------------------------------------------------
    TDirectory* dHist = fout->mkdir("histograms");
    dHist->cd();

    TH1D* hBx = new TH1D("hBx", "B_{x} distribution;B_{x} [mT];Entries", 200, bxr.first, bxr.second);
    TH1D* hBy = new TH1D("hBy", "B_{y} distribution;B_{y} [mT];Entries", 200, byr.first, byr.second);
    TH1D* hBz = new TH1D("hBz", "B_{z} distribution;B_{z} [mT];Entries", 200, bzr.first, bzr.second);
    TH1D* hBmag = new TH1D("hBmag", "|B| distribution;|B| [mT];Entries", 200, bmr.first, bmr.second);
    TH1D* hTemperature = new TH1D("hTemperature", "Temperature distribution;Temperature [^{o}C];Entries", 120, tr.first, tr.second);

    for (size_t i = 0; i < vbx.size(); ++i) {
        hBx->Fill(vbx[i]);
        hBy->Fill(vby[i]);
        hBz->Fill(vbz[i]);
        hBmag->Fill(vbmag[i]);
        hTemperature->Fill(vtemp[i]);
    }

    // ------------------------------------------------------------------
    // Temperature correlations and profiles.
    // ------------------------------------------------------------------
    TDirectory* dCorr = fout->mkdir("temperature_correlations");
    dCorr->cd();

    TH2D* hBxT = new TH2D("hBx_vs_T", "B_{x} vs temperature;Temperature [^{o}C];B_{x} [mT]",
                          80, tr.first, tr.second, 120, bxr.first, bxr.second);
    TH2D* hByT = new TH2D("hBy_vs_T", "B_{y} vs temperature;Temperature [^{o}C];B_{y} [mT]",
                          80, tr.first, tr.second, 120, byr.first, byr.second);
    TH2D* hBzT = new TH2D("hBz_vs_T", "B_{z} vs temperature;Temperature [^{o}C];B_{z} [mT]",
                          80, tr.first, tr.second, 120, bzr.first, bzr.second);
    TH2D* hBmagT = new TH2D("hBmag_vs_T", "|B| vs temperature;Temperature [^{o}C];|B| [mT]",
                            80, tr.first, tr.second, 120, bmr.first, bmr.second);

    TProfile* pBxT = new TProfile("pBx_vs_T", "Mean B_{x} vs temperature;Temperature [^{o}C];Mean B_{x} [mT]", 40, tr.first, tr.second);
    TProfile* pByT = new TProfile("pBy_vs_T", "Mean B_{y} vs temperature;Temperature [^{o}C];Mean B_{y} [mT]", 40, tr.first, tr.second);
    TProfile* pBzT = new TProfile("pBz_vs_T", "Mean B_{z} vs temperature;Temperature [^{o}C];Mean B_{z} [mT]", 40, tr.first, tr.second);
    TProfile* pBmagT = new TProfile("pBmag_vs_T", "Mean |B| vs temperature;Temperature [^{o}C];Mean |B| [mT]", 40, tr.first, tr.second);

    for (size_t i = 0; i < vbx.size(); ++i) {
        hBxT->Fill(vtemp[i], vbx[i]);
        hByT->Fill(vtemp[i], vby[i]);
        hBzT->Fill(vtemp[i], vbz[i]);
        hBmagT->Fill(vtemp[i], vbmag[i]);
        pBxT->Fill(vtemp[i], vbx[i]);
        pByT->Fill(vtemp[i], vby[i]);
        pBzT->Fill(vtemp[i], vbz[i]);
        pBmagT->Fill(vtemp[i], vbmag[i]);
    }

    // Preliminary apparent temperature slopes.
    TF1* fBxT = new TF1("fit_Bx_vs_T", "pol1", tr.first, tr.second);
    TF1* fByT = new TF1("fit_By_vs_T", "pol1", tr.first, tr.second);
    TF1* fBzT = new TF1("fit_Bz_vs_T", "pol1", tr.first, tr.second);
    TF1* fBmagT = new TF1("fit_Bmag_vs_T", "pol1", tr.first, tr.second);

    pBxT->Fit(fBxT, "Q0");
    pByT->Fit(fByT, "Q0");
    pBzT->Fit(fBzT, "Q0");
    pBmagT->Fit(fBmagT, "Q0");

    // ------------------------------------------------------------------
    // Graphs.
    // ------------------------------------------------------------------
    TDirectory* dGraphs = fout->mkdir("graphs");
    dGraphs->cd();

    TGraph* gBxT = new TGraph(nRows, vtemp.data(), vbx.data());
    TGraph* gByT = new TGraph(nRows, vtemp.data(), vby.data());
    TGraph* gBzT = new TGraph(nRows, vtemp.data(), vbz.data());
    gBxT->SetName("gBx_vs_T");
    gByT->SetName("gBy_vs_T");
    gBzT->SetName("gBz_vs_T");

    TGraph* gBxTime = new TGraph(nRows, vt.data(), vbx.data());
    TGraph* gByTime = new TGraph(nRows, vt.data(), vby.data());
    TGraph* gBzTime = new TGraph(nRows, vt.data(), vbz.data());
    gBxTime->SetName("gBx_vs_time");
    gByTime->SetName("gBy_vs_time");
    gBzTime->SetName("gBz_vs_time");

    gBxT->SetMarkerColor(kRed);
    gByT->SetMarkerColor(kBlue);
    gBzT->SetMarkerColor(kGreen+2);
    gBxT->SetMarkerStyle(20);
    gByT->SetMarkerStyle(20);
    gBzT->SetMarkerStyle(20);
    gBxT->SetMarkerSize(0.25);
    gByT->SetMarkerSize(0.25);
    gBzT->SetMarkerSize(0.25);

    gBxTime->SetLineColor(kRed);
    gByTime->SetLineColor(kBlue);
    gBzTime->SetLineColor(kGreen+2);
    gBxTime->SetLineWidth(1);
    gByTime->SetLineWidth(1);
    gBzTime->SetLineWidth(1);

    // ------------------------------------------------------------------
    // Canvases.
    // ------------------------------------------------------------------
    TDirectory* dCan = fout->mkdir("canvases");
    dCan->cd();

    // A) Raw scatter: Bx, By, Bz versus temperature.
    TCanvas* cBT = new TCanvas("c_B_components_vs_T", "B components vs temperature", 1200, 750);
    TH1F* frameBT = cBT->DrawFrame(tr.first, allBr.first, tr.second, allBr.second,
                                  "Magnetic field vs temperature;Temperature [^{o}C];B [mT]");
    frameBT->SetStats(0);
    gBxT->Draw("P SAME");
    gByT->Draw("P SAME");
    gBzT->Draw("P SAME");
    TLegend* l1 = new TLegend(0.80, 0.75, 0.93, 0.90);
    l1->AddEntry(gBxT, "B_{x}", "p");
    l1->AddEntry(gByT, "B_{y}", "p");
    l1->AddEntry(gBzT, "B_{z}", "p");
    l1->Draw();
    cBT->SetGrid();
    cBT->Modified();
    cBT->Update();

    // B) FIXED combined profile plot.
    // A common frame is explicitly created because the three TProfiles have
    // very different absolute means. This avoids the empty/incorrect overlay
    // produced when the first TProfile alone defines the Y axis.
    TCanvas* cProf = new TCanvas("c_profiles_vs_T", "Mean B vs temperature", 1200, 750);
    TH1F* frameProf = cProf->DrawFrame(tr.first, allBr.first, tr.second, allBr.second,
                                      "Mean magnetic field vs temperature;Temperature [^{o}C];Mean B [mT]");
    frameProf->SetStats(0);

    pBxT->SetLineColor(kRed);
    pByT->SetLineColor(kBlue);
    pBzT->SetLineColor(kGreen+2);
    pBxT->SetMarkerColor(kRed);
    pByT->SetMarkerColor(kBlue);
    pBzT->SetMarkerColor(kGreen+2);
    pBxT->SetMarkerStyle(20);
    pByT->SetMarkerStyle(21);
    pBzT->SetMarkerStyle(22);
    pBxT->SetMarkerSize(0.9);
    pByT->SetMarkerSize(0.9);
    pBzT->SetMarkerSize(0.9);

    pBxT->Draw("E1 SAME");
    pByT->Draw("E1 SAME");
    pBzT->Draw("E1 SAME");

    // Draw the preliminary linear fits using the same component colours.
    fBxT->SetLineColor(kRed);
    fByT->SetLineColor(kBlue);
    fBzT->SetLineColor(kGreen+2);
    fBxT->SetLineStyle(2);
    fByT->SetLineStyle(2);
    fBzT->SetLineStyle(2);
    fBxT->Draw("SAME");
    fByT->Draw("SAME");
    fBzT->Draw("SAME");

    TLegend* l2 = new TLegend(0.80, 0.72, 0.93, 0.90);
    l2->AddEntry(pBxT, "B_{x}", "lp");
    l2->AddEntry(pByT, "B_{y}", "lp");
    l2->AddEntry(pBzT, "B_{z}", "lp");
    l2->Draw();
    cProf->SetGrid();
    gPad->RedrawAxis();
    cProf->Modified();
    cProf->Update();

    // C) Bx, By, Bz versus elapsed time.
    TCanvas* cTime = new TCanvas("c_B_components_vs_time", "B components vs time", 1200, 750);
    TH1F* frameTime = cTime->DrawFrame(timer.first, allBr.first, timer.second, allBr.second,
                                       "Magnetic field stability;Elapsed time [s];B [mT]");
    frameTime->SetStats(0);
    gBxTime->Draw("L SAME");
    gByTime->Draw("L SAME");
    gBzTime->Draw("L SAME");
    TLegend* l3 = new TLegend(0.80, 0.75, 0.93, 0.90);
    l3->AddEntry(gBxTime, "B_{x}", "l");
    l3->AddEntry(gByTime, "B_{y}", "l");
    l3->AddEntry(gBzTime, "B_{z}", "l");
    l3->Draw();
    cTime->SetGrid();
    cTime->Modified();
    cTime->Update();

    // D) Individual 2D temperature correlation maps.
    TCanvas* cBxTH2 = new TCanvas("c_Bx_vs_T_2D", "Bx vs temperature", 1000, 750);
    hBxT->Draw("COLZ"); cBxTH2->SetGrid();
    TCanvas* cByTH2 = new TCanvas("c_By_vs_T_2D", "By vs temperature", 1000, 750);
    hByT->Draw("COLZ"); cByTH2->SetGrid();
    TCanvas* cBzTH2 = new TCanvas("c_Bz_vs_T_2D", "Bz vs temperature", 1000, 750);
    hBzT->Draw("COLZ"); cBzTH2->SetGrid();

    // ------------------------------------------------------------------
    // Export selected plots.
    // ------------------------------------------------------------------
    gSystem->mkdir(plotDir, true);
    cBT->SaveAs(Form("%s/B_components_vs_temperature.pdf", plotDir));
    cProf->SaveAs(Form("%s/B_profiles_vs_temperature.pdf", plotDir));
    cTime->SaveAs(Form("%s/B_components_vs_time.pdf", plotDir));
    cBxTH2->SaveAs(Form("%s/Bx_vs_temperature_2D.pdf", plotDir));
    cByTH2->SaveAs(Form("%s/By_vs_temperature_2D.pdf", plotDir));
    cBzTH2->SaveAs(Form("%s/Bz_vs_temperature_2D.pdf", plotDir));

    // ------------------------------------------------------------------
    // Metadata and output.
    // ------------------------------------------------------------------
    fout->cd();
    tree->Write();
    TParameter<Long64_t>("rows_written", nRows).Write();
    TParameter<Long64_t>("rows_skipped", nSkipped).Write();
    TParameter<double>("Bx_temp_slope_mT_per_C", fBxT->GetParameter(1)).Write();
    TParameter<double>("By_temp_slope_mT_per_C", fByT->GetParameter(1)).Write();
    TParameter<double>("Bz_temp_slope_mT_per_C", fBzT->GetParameter(1)).Write();
    TParameter<double>("Bmag_temp_slope_mT_per_C", fBmagT->GetParameter(1)).Write();
    TObjString(header.c_str()).Write("csv_header");

    fout->Write();
    fout->Close();

    std::cout << "\nCreated: " << outputRoot << std::endl;
    std::cout << "Rows written: " << nRows << std::endl;
    std::cout << "Rows skipped: " << nSkipped << std::endl;
    std::cout << "Apparent temperature slopes [mT/C]:\n"
              << "  Bx   = " << fBxT->GetParameter(1) << "\n"
              << "  By   = " << fByT->GetParameter(1) << "\n"
              << "  Bz   = " << fBzT->GetParameter(1) << "\n"
              << "  |B|  = " << fBmagT->GetParameter(1) << std::endl;
    std::cout << "Plots saved in: " << plotDir << std::endl;
}
