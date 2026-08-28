#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH1D.h>
#include <TH1F.h>
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

static void styleGraph(TGraph* g, Color_t color, Style_t marker)
{
    g->SetMarkerColor(color);
    g->SetLineColor(color);
    g->SetMarkerStyle(marker);
    g->SetMarkerSize(0.25);
    g->SetLineWidth(1);
}

static void styleProfile(TProfile* p, Color_t color, Style_t marker)
{
    p->SetLineColor(color);
    p->SetMarkerColor(color);
    p->SetMarkerStyle(marker);
    p->SetMarkerSize(0.9);
    p->SetLineWidth(2);
}

void make_nd280_analysis_v3(
    const char* inputCsv   = "nd280_2026-07-15_19-09-58(2).csv",
    const char* outputRoot = "nd280_2026-07-15_19-09-58_analysis.root",
    const char* plotDir    = "nd280_plots",
    int rebinFactor        = 8)
{
    // Keep histograms/profiles alive after the output ROOT file is closed.
    // This is important for interactive canvases: without this, a repaint of
    // a canvas after fout->Close() can appear blank even though the PDF saved
    // before closing the file is correct.
    TH1::AddDirectory(kFALSE);

    gStyle->SetOptStat(1110);
    gStyle->SetOptFit(1111);

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
            const double elapsed_s    = std::stod(f[1]);
            const double temperature  = std::stod(f[6]);
            const double bx           = std::stod(f[7]);
            const double by           = std::stod(f[8]);
            const double bz           = std::stod(f[9]);
            const double bmag         = std::sqrt(bx*bx + by*by + bz*bz);

            vt.push_back(elapsed_s);
            vtemp.push_back(temperature);
            vbx.push_back(bx);
            vby.push_back(by);
            vbz.push_back(bz);
            vbmag.push_back(bmag);
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

    const auto mmT    = minmax(vtemp);
    const auto mmBx   = minmax(vbx);
    const auto mmBy   = minmax(vby);
    const auto mmBz   = minmax(vbz);
    const auto mmBm   = minmax(vbmag);
    const auto mmTime = minmax(vt);

    const auto tr    = paddedRange(*mmT.first,    *mmT.second);
    const auto bxr   = paddedRange(*mmBx.first,   *mmBx.second);
    const auto byr   = paddedRange(*mmBy.first,   *mmBy.second);
    const auto bzr   = paddedRange(*mmBz.first,   *mmBz.second);
    const auto bmr   = paddedRange(*mmBm.first,   *mmBm.second);
    const auto timer = paddedRange(*mmTime.first, *mmTime.second, 0.01);

    const double allBmin = std::min({*mmBx.first, *mmBy.first, *mmBz.first});
    const double allBmax = std::max({*mmBx.second, *mmBy.second, *mmBz.second});
    const auto allBr = paddedRange(allBmin, allBmax, 0.10);

    std::cout << "Rows read: " << nRows << "   skipped: " << nSkipped << std::endl;
    std::cout << "Temperature range: " << *mmT.first << " to " << *mmT.second << " C" << std::endl;
    std::cout << "Gaussian histogram rebin factor: " << rebinFactor << std::endl;

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

    fout->cd();
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

    // Re-binned copies used only for Gaussian-fit characterization.
    // Default rebinFactor=8 reproduces the coarse binning used in the
    // preliminary plots. The original 200-bin histograms are preserved.
    TH1D* hBxFit = (TH1D*)hBx->Clone("hBx_gaussian_fit");
    TH1D* hByFit = (TH1D*)hBy->Clone("hBy_gaussian_fit");
    TH1D* hBzFit = (TH1D*)hBz->Clone("hBz_gaussian_fit");
    hBxFit->SetTitle("B_{x} distribution - Gaussian fit;B_{x} [mT];Entries");
    hByFit->SetTitle("B_{y} distribution - Gaussian fit;B_{y} [mT];Entries");
    hBzFit->SetTitle("B_{z} distribution - Gaussian fit;B_{z} [mT];Entries");

    if (rebinFactor > 1) {
        hBxFit->Rebin(rebinFactor);
        hByFit->Rebin(rebinFactor);
        hBzFit->Rebin(rebinFactor);
    }

    TF1* fBxG = new TF1("fit_Bx_gaus", "gaus", hBxFit->GetXaxis()->GetXmin(), hBxFit->GetXaxis()->GetXmax());
    TF1* fByG = new TF1("fit_By_gaus", "gaus", hByFit->GetXaxis()->GetXmin(), hByFit->GetXaxis()->GetXmax());
    TF1* fBzG = new TF1("fit_Bz_gaus", "gaus", hBzFit->GetXaxis()->GetXmin(), hBzFit->GetXaxis()->GetXmax());
    // Robust initial parameters for the Gaussian minimisation.
    fBxG->SetParameters(hBxFit->GetMaximum(), hBxFit->GetMean(), hBxFit->GetRMS());
    fByG->SetParameters(hByFit->GetMaximum(), hByFit->GetMean(), hByFit->GetRMS());
    fBzG->SetParameters(hBzFit->GetMaximum(), hBzFit->GetMean(), hBzFit->GetRMS());

    fBxG->SetLineColor(kRed);
    fByG->SetLineColor(kRed);
    fBzG->SetLineColor(kRed);
    fBxG->SetLineWidth(3);
    fByG->SetLineWidth(3);
    fBzG->SetLineWidth(3);

    // Whole-range Gaussian fits, matching the preliminary characterization.
    hBxFit->Fit(fBxG, "Q0");
    hByFit->Fit(fByG, "Q0");
    hBzFit->Fit(fBzG, "Q0");

    // ------------------------------------------------------------------
    // Temperature correlations and profiles.
    // ------------------------------------------------------------------
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
    TGraph* gBxT = new TGraph(nRows, vtemp.data(), vbx.data());
    TGraph* gByT = new TGraph(nRows, vtemp.data(), vby.data());
    TGraph* gBzT = new TGraph(nRows, vtemp.data(), vbz.data());
    gBxT->SetName("gBx_vs_T");
    gByT->SetName("gBy_vs_T");
    gBzT->SetName("gBz_vs_T");
    styleGraph(gBxT, kRed, 20);
    styleGraph(gByT, kBlue, 20);
    styleGraph(gBzT, kGreen+2, 20);

    TGraph* gBxTime = new TGraph(nRows, vt.data(), vbx.data());
    TGraph* gByTime = new TGraph(nRows, vt.data(), vby.data());
    TGraph* gBzTime = new TGraph(nRows, vt.data(), vbz.data());
    gBxTime->SetName("gBx_vs_time");
    gByTime->SetName("gBy_vs_time");
    gBzTime->SetName("gBz_vs_time");
    gBxTime->SetLineColor(kRed);
    gByTime->SetLineColor(kBlue);
    gBzTime->SetLineColor(kGreen+2);
    gBxTime->SetLineWidth(1);
    gByTime->SetLineWidth(1);
    gBzTime->SetLineWidth(1);

    styleProfile(pBxT, kRed, 20);
    styleProfile(pByT, kBlue, 21);
    styleProfile(pBzT, kGreen+2, 22);

    fBxT->SetLineColor(kRed);
    fByT->SetLineColor(kBlue);
    fBzT->SetLineColor(kGreen+2);
    fBxT->SetLineStyle(2);
    fByT->SetLineStyle(2);
    fBzT->SetLineStyle(2);

    // ------------------------------------------------------------------
    // Canvases. Because all histogram-like objects are detached from the
    // TFile, these remain valid and repaint correctly after fout->Close().
    // ------------------------------------------------------------------

    // A) Raw scatter: Bx, By, Bz versus temperature.
    TCanvas* cBT = new TCanvas("c_B_components_vs_T", "B components vs temperature", 1200, 750);
    cBT->cd();
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

    // B) Mean profiles Bx, By, Bz versus temperature.
    TCanvas* cProf = new TCanvas("c_profiles_vs_T", "Mean B vs temperature", 1200, 750);
    cProf->cd();
    TH1F* frameProf = cProf->DrawFrame(tr.first, allBr.first, tr.second, allBr.second,
                                      "Mean magnetic field vs temperature;Temperature [^{o}C];Mean B [mT]");
    frameProf->SetStats(0);
    pBxT->Draw("E1 SAME");
    pByT->Draw("E1 SAME");
    pBzT->Draw("E1 SAME");
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
    cTime->cd();
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
    cBxTH2->cd(); hBxT->Draw("COLZ"); cBxTH2->SetGrid(); cBxTH2->Update();
    TCanvas* cByTH2 = new TCanvas("c_By_vs_T_2D", "By vs temperature", 1000, 750);
    cByTH2->cd(); hByT->Draw("COLZ"); cByTH2->SetGrid(); cByTH2->Update();
    TCanvas* cBzTH2 = new TCanvas("c_Bz_vs_T_2D", "Bz vs temperature", 1000, 750);
    cBzTH2->cd(); hBzT->Draw("COLZ"); cBzTH2->SetGrid(); cBzTH2->Update();

    // E) Gaussian fits on re-binned Bx/By/Bz distributions.
    TCanvas* cBxG = new TCanvas("c_Bx_gaussian_fit", "Bx Gaussian fit", 900, 700);
    cBxG->cd();
    hBxFit->SetFillColor(kYellow);
    hBxFit->SetLineColor(kBlue+1);
    hBxFit->Draw("HIST");
    fBxG->Draw("SAME");
    cBxG->Modified(); cBxG->Update();

    TCanvas* cByG = new TCanvas("c_By_gaussian_fit", "By Gaussian fit", 900, 700);
    cByG->cd();
    hByFit->SetFillColor(kYellow);
    hByFit->SetLineColor(kBlue+1);
    hByFit->Draw("HIST");
    fByG->Draw("SAME");
    cByG->Modified(); cByG->Update();

    TCanvas* cBzG = new TCanvas("c_Bz_gaussian_fit", "Bz Gaussian fit", 900, 700);
    cBzG->cd();
    hBzFit->SetFillColor(kYellow);
    hBzFit->SetLineColor(kBlue+1);
    hBzFit->Draw("HIST");
    fBzG->Draw("SAME");
    cBzG->Modified(); cBzG->Update();

    TCanvas* cGAll = new TCanvas("c_B_gaussian_fits", "B Gaussian fits", 1500, 500);
    cGAll->Divide(3,1);
    cGAll->cd(1); hBxFit->Draw("HIST"); fBxG->Draw("SAME");
    cGAll->cd(2); hByFit->Draw("HIST"); fByG->Draw("SAME");
    cGAll->cd(3); hBzFit->Draw("HIST"); fBzG->Draw("SAME");
    cGAll->Modified(); cGAll->Update();

    // ------------------------------------------------------------------
    // Export plots.
    // ------------------------------------------------------------------
    gSystem->mkdir(plotDir, true);
    cBT->SaveAs(Form("%s/B_components_vs_temperature.pdf", plotDir));
    cProf->SaveAs(Form("%s/B_profiles_vs_temperature.pdf", plotDir));
    cTime->SaveAs(Form("%s/B_components_vs_time.pdf", plotDir));
    cBxTH2->SaveAs(Form("%s/Bx_vs_temperature_2D.pdf", plotDir));
    cByTH2->SaveAs(Form("%s/By_vs_temperature_2D.pdf", plotDir));
    cBzTH2->SaveAs(Form("%s/Bz_vs_temperature_2D.pdf", plotDir));
    cBxG->SaveAs(Form("%s/Bx_distribution_gaussian_fit.pdf", plotDir));
    cByG->SaveAs(Form("%s/By_distribution_gaussian_fit.pdf", plotDir));
    cBzG->SaveAs(Form("%s/Bz_distribution_gaussian_fit.pdf", plotDir));
    cGAll->SaveAs(Form("%s/Bxyz_distributions_gaussian_fits.pdf", plotDir));

    // ------------------------------------------------------------------
    // Write ROOT structure explicitly. The histograms/profiles are not
    // file-owned, so explicit Write() calls preserve them while allowing
    // the interactive canvases to remain alive after the file is closed.
    // ------------------------------------------------------------------
    fout->cd();
    tree->Write();
    TParameter<Long64_t>("rows_written", nRows).Write();
    TParameter<Long64_t>("rows_skipped", nSkipped).Write();
    TParameter<int>("gaussian_rebin_factor", rebinFactor).Write();
    TParameter<double>("Bx_temp_slope_mT_per_C", fBxT->GetParameter(1)).Write();
    TParameter<double>("By_temp_slope_mT_per_C", fByT->GetParameter(1)).Write();
    TParameter<double>("Bz_temp_slope_mT_per_C", fBzT->GetParameter(1)).Write();
    TParameter<double>("Bmag_temp_slope_mT_per_C", fBmagT->GetParameter(1)).Write();
    TObjString(header.c_str()).Write("csv_header");

    TDirectory* dHist = fout->mkdir("histograms");
    dHist->cd();
    hBx->Write(); hBy->Write(); hBz->Write(); hBmag->Write(); hTemperature->Write();

    TDirectory* dGF = fout->mkdir("gaussian_fits");
    dGF->cd();
    hBxFit->Write(); hByFit->Write(); hBzFit->Write();
    fBxG->Write(); fByG->Write(); fBzG->Write();

    TDirectory* dCorr = fout->mkdir("temperature_correlations");
    dCorr->cd();
    hBxT->Write(); hByT->Write(); hBzT->Write(); hBmagT->Write();
    pBxT->Write(); pByT->Write(); pBzT->Write(); pBmagT->Write();
    fBxT->Write(); fByT->Write(); fBzT->Write(); fBmagT->Write();

    TDirectory* dGraphs = fout->mkdir("graphs");
    dGraphs->cd();
    gBxT->Write(); gByT->Write(); gBzT->Write();
    gBxTime->Write(); gByTime->Write(); gBzTime->Write();

    TDirectory* dCan = fout->mkdir("canvases");
    dCan->cd();
    cBT->Write(); cProf->Write(); cTime->Write();
    cBxTH2->Write(); cByTH2->Write(); cBzTH2->Write();
    cBxG->Write(); cByG->Write(); cBzG->Write(); cGAll->Write();

    fout->Write();
    fout->Close();

    // Force final repaint after the ROOT file has been closed.
    cBT->Modified(); cBT->Update();
    cProf->Modified(); cProf->Update();
    cTime->Modified(); cTime->Update();
    cBxTH2->Modified(); cBxTH2->Update();
    cByTH2->Modified(); cByTH2->Update();
    cBzTH2->Modified(); cBzTH2->Update();
    cBxG->Modified(); cBxG->Update();
    cByG->Modified(); cByG->Update();
    cBzG->Modified(); cBzG->Update();
    cGAll->Modified(); cGAll->Update();

    std::cout << "\nCreated: " << outputRoot << std::endl;
    std::cout << "Rows written: " << nRows << std::endl;
    std::cout << "Rows skipped: " << nSkipped << std::endl;
    std::cout << "Apparent temperature slopes [mT/C]:\n"
              << "  Bx   = " << fBxT->GetParameter(1) << "\n"
              << "  By   = " << fByT->GetParameter(1) << "\n"
              << "  Bz   = " << fBzT->GetParameter(1) << "\n"
              << "  |B|  = " << fBmagT->GetParameter(1) << std::endl;
    std::cout << "Gaussian fits (rebin=" << rebinFactor << "):\n"
              << "  Bx mean=" << fBxG->GetParameter(1) << " mT, sigma=" << fBxG->GetParameter(2) << " mT\n"
              << "  By mean=" << fByG->GetParameter(1) << " mT, sigma=" << fByG->GetParameter(2) << " mT\n"
              << "  Bz mean=" << fBzG->GetParameter(1) << " mT, sigma=" << fBzG->GetParameter(2) << " mT" << std::endl;
    std::cout << "Plots saved in: " << plotDir << std::endl;
}
