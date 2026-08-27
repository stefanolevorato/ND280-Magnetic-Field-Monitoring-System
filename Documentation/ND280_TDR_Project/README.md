# ND280 Magnetic Field Monitoring System - TDR v0.5

This package contains the LaTeX source and compiled PDF of the Technical Design Report.

## v0.5 update

- Added calibration and metrological requirements to Chapter 3.
- Added a dedicated Calibration and Metrological Characterization Strategy to Chapter 11.
- Added a planned Rev.A calibration work package to the roadmap.
- Added a bibliography entry for Messner et al., *Calibration and characterization of a 3D magnetic Hall sensor card* (prepared for submission to JINST).
- The TDR explicitly distinguishes the ND280 long-term monitoring objective from high-precision absolute magnetic-field mapping while adopting the relevant calibration methodology: per-sensor traceability, vector correction, temperature characterization, multi-field checks and reproducibility studies.

## Build

    pdflatex main.tex
    bibtex main
    pdflatex main.tex
    pdflatex main.tex
