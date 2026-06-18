DOCS_DIR := docs

TUTOR_MISSION_NAME := gnubg_mobile_tutor_mission_statement
TUTOR_MISSION_TEX := $(DOCS_DIR)/$(TUTOR_MISSION_NAME).tex
TUTOR_MISSION_PDF := $(DOCS_DIR)/$(TUTOR_MISSION_NAME).pdf

.PHONY: docs tutor-mission-pdf clean-docs clean-docs-all

docs: tutor-mission-pdf

tutor-mission-pdf: $(TUTOR_MISSION_PDF)

$(TUTOR_MISSION_PDF): $(TUTOR_MISSION_TEX)
	@if command -v latexmk >/dev/null 2>&1; then \
		latexmk \
			-pdf \
			-interaction=nonstopmode \
			-halt-on-error \
			-output-directory=$(DOCS_DIR) \
			$(TUTOR_MISSION_TEX); \
	else \
		pdflatex \
			-interaction=nonstopmode \
			-halt-on-error \
			-output-directory=$(DOCS_DIR) \
			$(TUTOR_MISSION_TEX); \
		pdflatex \
			-interaction=nonstopmode \
			-halt-on-error \
			-output-directory=$(DOCS_DIR) \
			$(TUTOR_MISSION_TEX); \
	fi
	@$(MAKE) clean-docs

clean-docs:
	rm -f $(DOCS_DIR)/*.aux
	rm -f $(DOCS_DIR)/*.fdb_latexmk
	rm -f $(DOCS_DIR)/*.fls
	rm -f $(DOCS_DIR)/*.log
	rm -f $(DOCS_DIR)/*.out
	rm -f $(DOCS_DIR)/*.toc

clean-docs-all: clean-docs
	rm -f $(TUTOR_MISSION_PDF)
