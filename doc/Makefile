# doc/Makefile
#
# Usage:
#   make          — build current version PDF (default)
#   make all      — build all versions
#   make clean    — remove build artefacts
#   make view     — build current PDF and open it
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

VERSIONS := \
	MASTER_V0.8.1 \
	MASTER_V0.8.2 \
	MASTER_V0.8.3 \
	MASTER_V0.8.4 \
	MASTER_V0.8.5

PDFS := $(addsuffix .pdf, $(VERSIONS))

.PHONY: all clean view

all: $(PDFS)

$(CURRENT).pdf: $(CURRENT).tex
	$(PDF_ENGINE) -interaction=nonstopmode $
	$(PDF_ENGINE) -interaction=nonstopmode $
	@echo "Built: $@"

%.pdf: %.tex
	$(PDF_ENGINE) -interaction=nonstopmode $
	$(PDF_ENGINE) -interaction=nonstopmode $
	@echo "Built: $@"

clean:
	rm -f *.aux *.log *.out *.toc *.pdf

view: $(CURRENT).pdf
	xdg-open $(CURRENT).pdf &
