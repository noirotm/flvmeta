man_MANS = $(builddir)/flvmeta.1

CLEANFILES = $(builddir)/flvmeta.1

flvmeta.1: $(srcdir)/flvmeta.1.md
	pandoc -s $(srcdir)/flvmeta.1.md -o $(builddir)/flvmeta.1

EXTRA_DIST = $(srcdir)/flvmeta.1.md
