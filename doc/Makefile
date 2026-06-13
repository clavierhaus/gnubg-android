# doc/Makefile
#
# Usage:
#   make          — build PDF (default)
#   make pdf      — build PDF
#   make tex      — build .tex intermediate only
#   make all      — build both
#   make clean    — remove build artefacts
#   make view     — build PDF and open it
#
# Requirements (Fedora 44):
#   sudo dnf install pandoc texlive-xetex texlive-tex-gyre \
#                    texlive-collection-fontsrecommended \
#                    texlive-titlesec texlive-fvextra texlive-framed \
#                    texlive-booktabs texlive-microtype texlive-multirow \
#                    texlive-fancyhdr texlive-parskip
#
# Fonts used:
#   TeX Gyre Pagella  — body text (Palatino-style serif)
#   DejaVu Sans Mono  — code blocks

PANDOC     := pandoc
PDF_ENGINE := xelatex

SRC        := ../MASTER_V8.md
TEMPLATE   := gnubg.tex
OUT_TEX    := MASTER_V8.tex
OUT_PDF    := MASTER_V8.pdf

PANDOC_FLAGS := \
	--from markdown \
	--template $(TEMPLATE) \
	--highlight-style tango \
	--table-of-contents \
	--toc-depth=3 \
	--number-sections \
	--metadata title="GNU Backgammon Android Port" \
	--metadata author="clavierhaus.at" \
	--metadata date="$(shell date +'%B %Y')"

.PHONY: all pdf tex clean view

all: pdf tex

pdf: $(OUT_PDF)

tex: $(OUT_TEX)

$(OUT_PDF): $(SRC) $(TEMPLATE)
	$(PANDOC) $(PANDOC_FLAGS) \
		--to pdf \
		--pdf-engine $(PDF_ENGINE) \
		$< -o $@
	@echo "Built: $@"

$(OUT_TEX): $(SRC) $(TEMPLATE)
	$(PANDOC) $(PANDOC_FLAGS) \
		--to latex \
		$< -o $@
	@echo "Built: $@"

clean:
	rm -f $(OUT_PDF) $(OUT_TEX)

view: $(OUT_PDF)
	xdg-open $(OUT_PDF) &
