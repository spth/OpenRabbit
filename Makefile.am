# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

if ENABLE_COLDBOOT
  COLDBOOT = coldboot
else
  COLDBOOT =
endif

SUBDIRS = src $(COLDBOOT)

dist_doc_DATA = README ChangeLog

