TARGET		:= manual
INCLUDE		:= czechiso.bst

.PHONY: clean bibtex pdflatex

$(TARGET).pdf: $(TARGET).ps
	ps2pdf -sPAPERSIZE=a4 $^ ../$(TARGET).pdf

$(TARGET).ps: $(TARGET).dvi
	dvips $^

$(TARGET).dvi: $(TARGET).tex
	latex $^
	make bibtex
	latex $^
	latex $^

bibtex: $(TARGET).bib
	bibtex $(TARGET)

pdflatex: $(TARGET).tex
	pdflatex $^
	make bibtex
	pdflatex $^
	pdflatex $^

clean:
	rm -f $(TARGET).aux $(TARGET).dvi $(TARGET).log $(TARGET).ps $(TARGET).synctex.gz $(TARGET).fls $(TARGET).fdb_latexmk $(TARGET).bbl $(TARGET).blg
