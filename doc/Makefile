# doc/Makefile
#
# Usage:
#   make          — build current version PDF
#   make all      — build current version PDF
#   make clean    — remove build artefacts
#   make view     — build and open current PDF
#
# Requirements (Fedora 44):
#   sudo dnf install texlive-xetex texlive-tex-gyre \
#                    texlive-collection-fontsrecommended \
#                    texlive-titlesec texlive-fvextra texlive-framed \
#                    texlive-booktabs texlive-microtype texlive-multirow \
#                    texlive-fancyhdr texlive-parskip texlive-hyperref
#
# Fonts used:
#   TeX Gyre Pagella  — body text (Palatino-style serif)
#   DejaVu Sans Mono  — code blocks

PDF_ENGINE := xelatex
CURRENT    := MASTER_V0.8.5

.PHONY: all clean view

all: $(CURRENT).pdf

$(CURRENT).pdf: $(CURRENT).tex
	$(PDF_ENGINE) -interaction=nonstopmode $(CURRENT).tex
	$(PDF_ENGINE) -interaction=nonstopmode $(CURRENT).tex
	@echo "Built: $(CURRENT).pdf"

clean:
	rm -f *.aux *.log *.out *.toc

view: $(CURRENT).pdf
	xdg-open $(CURRENT).pdf &
