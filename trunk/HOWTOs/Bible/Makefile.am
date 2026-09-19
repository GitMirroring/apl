# Preview of "The GNU APL Bible" (front cover, table of contents for
# the ENTIRE book, preface, Part I / Chapters 1-3 in full, and
# placeholder ordering information), plus this README.
#
# The TOC is an exact copy of the full book's own TOC -- every Part and
# Chapter, same styling throughout, with the SAME PAGE NUMBERS as the
# real book.pdf (not this preview's own, much shorter pagination) --
# built by build_toc.py, which reads titles/structure straight from
# ../../Book/book.adoc and each ../../Book/chNN.adoc (never retyped)
# and real page numbers from ../../Book/book.pdf's own outline. A
# reader sees exactly the book they would be ordering. Chapters 1-3 are
# real, linked entries (they are actually in this PDF); the rest are
# the same markup minus the link, since there is no page here to link
# to. The preview-only "How to Order" section is deliberately left out
# of the TOC (it is not part of the book itself), though the section
# still exists at the end of the PDF.
#
# Only README and Bible-Preview.pdf are distributed/exported (Savannah
# is public; the Book/ sources this preview builds from -- one directory
# up -- are not). Bible-Preview.adoc, ordering.adoc, build_toc.py and
# images/front_cover.png stay local-SVN-only: they are build sources,
# tracked so the PDF can be rebuilt here, but deliberately left out of
# dist_doc_Bible_DATA below.
#
# The build reuses the same asciidoc/weasyprint pipeline as Book/Makefile,
# trimmed to the fixups this excerpt actually needs -- see that Makefile
# for the full book's own (much larger) pipeline and the reasoning
# behind each fixup; this is a curated subset of it, not an independent
# design.
#
# Unlike the full book, this preview is a digital-only handout (not sent
# to print-on-demand), so the full book's grayscale-for-print-cost CSS
# pass is deliberately NOT reused here -- syntax-highlighted code looks
# better in colour for this purpose.

# dist_doc_Bible_DATA = \
#   README            \
#   Bible-Preview.pdf

doc_Bibledir = $(docdir)/Bible

PREVIEW_DEPS = \
    Bible-Preview.adoc \
    ordering.adoc \
    build_toc.py \
    images/front_cover.png \
    ../../Book/book.adoc \
    ../../Book/book.pdf \
    ../../Book/preface.adoc \
    ../../Book/ch01.adoc \
    ../../Book/ch02.adoc \
    ../../Book/ch03.adoc

# all: Bible-Preview.pdf

# front_cover.png is a checked-in crop of Book/cover/cover_print_rgb.pdf's
# front panel (the finished cover artwork, trim size only, no bleed) --
# regenerate it if the cover ever changes:
#
#   cd ../../Book/cover && pdftoppm -r 300 -x 2798 -y 59 -W 2480 -H 3508 \
#       -png cover_print_rgb.pdf front_cover_only
#   mv front_cover_only-1.png ../../HOWTOs/Bible/images/front_cover.png
#
# (2798/59/2480/3508 are FRONT_X/BLEED/TRIM_W/TRIM_H from build_cover.py,
# converted from mm to 300dpi pixels; recompute if that geometry changes.)

# Classic asciidoc's [.role]#...# quoted-text span leaves the role's
# leading "." literally in the emitted class attribute when the span's
# content is an inline macro (e.g. link:...[]) rather than plain text --
# ordering.adoc's [.order-link]#link:...[]# is the only such case in
# this document; the recipe below strips the stray dot by hand.
Bible-Preview.html: $(PREVIEW_DEPS)
	@if [ -z "`which asciidoc`" ] || ! python3 -c "import pypdf" 2>/dev/null ; then touch $@ ; else \
	asciidoc -a icons -a source-highlighter=pygments -o Bible-Preview.html Bible-Preview.adoc ; \
	sed -i 's|<pre><code>|<pre>\n\n<code>|'     Bible-Preview.html ; \
	sed -i 's|</code></pre>|</code>\n\n</pre>|' Bible-Preview.html ; \
	grep -v -F "<noscript>" Bible-Preview.html      > /tmp/Bible-Preview.html ; \
	mv -f /tmp/Bible-Preview.html                     Bible-Preview.html ; \
	sed -i 's/\.monospaced, code, pre {/code { font-weight: bold; }\npre code { font-weight: normal; }\n.monospaced, code, pre {/' Bible-Preview.html ; \
	sed -i 's/font-family: "Courier New", Courier, monospace;/font-family: "BQN386 Unicode", "DejaVu Sans Mono", monospace;/' Bible-Preview.html ; \
	sed -i 's|</style>|div.tableblock > table { border-collapse: collapse; }\ndiv.tableblock table td, div.tableblock table th { border: 1px solid #527bbd; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|div.tableblock > table { border-width: 2.25px; }\n</style>|' Bible-Preview.html ; \
	python3 -c "import re; s=open('Bible-Preview.html',encoding='utf-8').read(); s=re.sub(r'<col width=\"([0-9]+%)\" />', r'<col style=\"width:\1\" />', s); open('Bible-Preview.html','w',encoding='utf-8').write(s)" ; \
	sed -i 's|</style>|.highlight .err { border: none; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|span.heading-lead { text-decoration: underline; font-weight: normal; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|code, .monospaced { white-space: nowrap; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|pre code { white-space: pre; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|div.imageblock img { max-width: 100%; height: auto; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|pre, pre code { font-size: 9pt; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|white-space: pre-wrap;|white-space: pre;|' Bible-Preview.html ; \
	sed -i 's|</style>|@page { @bottom-center { content: counter(page); font-size: 9pt; font-family: sans-serif; } margin-bottom: 20mm; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print { a, a:visited { color: inherit; text-decoration: none; } }\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|.order-link a, .order-link a:visited { color: #1a4fa0 !important; font-weight: bold !important; }\n</style>|' Bible-Preview.html ; \
	sed -i 's|class="\.order-link"|class="order-link"|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  p { orphans: 4; widows: 4; }\n  div.imageblock, div.tableblock { break-inside: avoid; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  div.tableblock tr { break-inside: avoid; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  div.listingblock { break-inside: avoid; }\n  div.literalblock { break-inside: avoid; }\n  div.exampleblock { break-inside: avoid; }\n  div.paragraph + div.listingblock { break-before: avoid; }\n  div.paragraph + div.tableblock { break-before: avoid; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  h1, h2, h3, h4, h5, h6 { break-after: avoid; }\n  h1 + *, h2 + *, h3 + *, h4 + *, h5 + *, h6 + * { break-before: avoid; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  h1.float { break-before: always; }\n  h2 { break-before: always; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|#header, #footer { display: none; }\ndiv.imageblock.cover-page { margin: 0; text-align: center; }\ndiv.imageblock.cover-page div.content, div.imageblock.cover-page img { margin: 0; width: 100%; max-width: 100%; height: auto; display: block; }\n@page :first { margin: 0; @bottom-center { content: none; } }\n</style>|' Bible-Preview.html ; \
	sed -i 's|#toctitle {|#toctitle {\n  text-align: center;|' Bible-Preview.html ; \
	sed -i 's|</style>|@media print {\n  #toc { break-before: always; }\n}\n</style>|' Bible-Preview.html ; \
	sed -i 's|</style>|div.toclevel1, div.toclevel2 { display: flex; align-items: baseline; }\n.toc-title { flex: 0 1 auto; }\n.toc-fill { flex: 1 1 auto; border-bottom: 1px dotted black; margin: 0 0.3em 0.2em; }\n.toc-page { flex: 0 0 auto; }\n</style>|' Bible-Preview.html ; \
	sed -E -z -i 's/em \{\r?\n  font-style: italic;\r?\n  color: navy;\r?\n\}/em {\n  font-style: italic;\n}/' Bible-Preview.html ; \
	sed -E -z -i 's/h1, h2, h3, h4, h5, h6 \{\r?\n  color: #527bbd;\r?\n  margin-top: 1\.2em;\r?\n  margin-bottom: 0\.5em;\r?\n  line-height: 1\.3;\r?\n\}/h1, h2, h3, h4, h5, h6 {\n  color: black;\n  margin-top: 1.2em;\n  margin-bottom: 0.5em;\n  line-height: 1.3;\n}/' Bible-Preview.html ; \
	sed -i 's|<title>\(.*\)<span style="white-space:nowrap">C++</span>|<title>\1C++|' Bible-Preview.html ; \
	python3 ../../Book/mark_first_chapters.py Bible-Preview.html ; \
	python3 build_toc.py Bible-Preview.html ; \
	fi

Bible-Preview.pdf: Bible-Preview.html
	@if [ -z "`which python3`" ] || ! python3 -c "import weasyprint" 2>/dev/null ; then \
	    touch $@ ; \
	else \
	    python3 -m weasyprint Bible-Preview.html Bible-Preview.pdf ; \
	fi

CLEANFILES = Bible-Preview.html Bible-Preview.pdf
